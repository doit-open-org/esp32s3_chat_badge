#ifndef SIMULATOR
#include "misc/lv_timer_private.h"
#include "core/lv_obj_class_private.h"
#include "widgets/image/lv_image_private.h"
#else
#include "src/misc/lv_timer_private.h"
#include "src/core/lv_obj_class_private.h"
#include "src/widgets/image/lv_image_private.h"
#endif
#include "lv_vpg_private.h"
#include "esp_log.h"
/*********************
 *      DEFINES
 *********************/
#define MY_CLASS (&lv_vpg_class)

/**********************
 *      TYPEDEFS
 **********************/

 /**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_fs_res_t vpg_file_read(void *ctx, void *buf, uint32_t size, uint32_t *out_read);
static lv_fs_res_t vpg_file_seek(void *ctx, uint32_t pos, uint8_t whence);
static lv_fs_res_t vpg_file_tell(void *ctx, uint32_t *pos);
static void vpg_file_close(void *ctx);
static lv_fs_res_t vpg_mem_read(void *ctx, void *buf, uint32_t size, uint32_t *out_read);
static lv_fs_res_t vpg_mem_seek(void *ctx, uint32_t pos, uint8_t whence);
static lv_fs_res_t vpg_mem_tell(void *ctx, uint32_t *pos);
static void vpg_mem_close(void *ctx);

static const vpg_io_t VPG_MEM_IO = {
    .read = vpg_mem_read,
    .seek = vpg_mem_seek,
    .tell = vpg_mem_tell,
    .close = vpg_mem_close,
};

static const vpg_io_t VPG_FILE_IO = {
    .read = vpg_file_read,
    .seek = vpg_file_seek,
    .tell = vpg_file_tell,
    .close = vpg_file_close,
};

static void lv_vpg_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_vpg_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void next_frame_task_cb(lv_timer_t * t);
static void vpg_close(vpg_t *vpg);
static void vpg_load_frame(vpg_t *vpg, uint8_t *frame);
static int vpg_get_frame(vpg_t *vpg);
static vpg_t *vpg_open(void *src);


/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_vpg_class = {
    .constructor_cb = lv_vpg_constructor,
    .destructor_cb = lv_vpg_destructor,
    .instance_size = sizeof(lv_vpg_t),
    .base_class = &lv_image_class,
    // .name = "vpg",
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_vpg_create(lv_obj_t * parent)
{

    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_vpg_set_src(lv_obj_t * obj, const void * src)
{
    lv_vpg_t * vpgobj = (lv_vpg_t *) obj;
    vpg_t *vpg = vpgobj->vpg;
    lv_timer_pause(vpgobj->timer);

    /*Close previous vpg if any*/
    if(vpg != NULL) {
        lv_image_cache_drop(lv_image_get_src(obj));

        vpg_close(vpg);
        vpgobj->vpg = NULL;
        vpgobj->imgdsc.data = NULL;
    }

    vpg = vpg_open(src);

    
    if(vpg == NULL) {
        lv_obj_send_event(obj, LV_EVENT_CANCEL, NULL);
        LV_LOG_WARN("Couldn't load the source");
        return;
    }

    vpgobj->vpg = vpg;
    vpgobj->imgdsc.data = vpg->frame;
    vpgobj->imgdsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    vpgobj->imgdsc.header.flags = LV_IMAGE_FLAGS_MODIFIABLE;
    vpgobj->imgdsc.header.cf = LV_COLOR_FORMAT_RAW;
    vpgobj->imgdsc.header.h = vpg->height;
    vpgobj->imgdsc.header.w = vpg->width;
    vpgobj->imgdsc.data_size = vpg->frame_size;

    vpgobj->last_call = lv_tick_get();

    lv_image_set_src(obj, &vpgobj->imgdsc);

    lv_timer_resume(vpgobj->timer);
    lv_timer_reset(vpgobj->timer);

    next_frame_task_cb(vpgobj->timer);
}


static void lv_vpg_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_vpg_t * vpgobj = (lv_vpg_t *) obj;

    vpgobj->vpg = NULL;
    vpgobj->timer = lv_timer_create(next_frame_task_cb, 10, obj);
    lv_timer_pause(vpgobj->timer);
}

static void lv_vpg_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_vpg_t * vpgobj = (lv_vpg_t *) obj;

    lv_image_cache_drop(lv_image_get_src(obj));

    if(vpgobj->vpg)
        vpg_close(vpgobj->vpg);
    lv_timer_delete(vpgobj->timer);
}

static void next_frame_task_cb(lv_timer_t * t)
{
    lv_obj_t * obj = t->user_data;
    lv_vpg_t * vpgobj = (lv_vpg_t *) obj;
    uint32_t elaps = lv_tick_elaps(vpgobj->last_call);
    if(elaps < vpgobj->vpg->delay_ms) return;

    if (vpgobj->vpg == NULL)
    {
        lv_timer_pause(t);
        return;
    }

    vpgobj->last_call = lv_tick_get();

    // // 如果已经是最后一帧，暂停播放
    // if (vpgobj->vpg->index == vpgobj->vpg->vpg->header.itemNum - 1)
    // {
    //     ESP_LOGI("VPG", "VPG reached last frame (%lu), pausing",
    //              (unsigned long)(vpgobj->vpg->index + 1));
    //     lv_timer_pause(t);
    //     lv_obj_send_event(obj, LV_EVENT_READY, NULL);
    //     return;
    // }

    // // 如果已经播放到第一帧（循环回来），暂停播放
    // if (vpgobj->vpg->index == 0)
    // {
    //     ESP_LOGI("VPG", "VPG reached first frame, pausing");
    //     lv_timer_pause(t);
    //     lv_obj_send_event(obj, LV_EVENT_READY, NULL);
    //     return;
    // }

    int has_next = vpg_get_frame(vpgobj->vpg);

//    //打印帧信息
//    ESP_LOGI("VPG", "VPG frame: %d/%d, size: %d, delay: %dms",
//             vpgobj->vpg->index + 1,
//             vpgobj->vpg->vpg->header.itemNum,
//             vpgobj->vpg->vpg->item[vpgobj->vpg->index].size,
//             vpgobj->vpg->delay_ms);
    if(has_next == 0) {
        /*It was the last repeat*/
        lv_result_t res = lv_obj_send_event(obj, LV_EVENT_READY, NULL);
        lv_timer_pause(t);
        if(res != LV_RESULT_OK) return;
    }
    if (vpgobj->vpg->index == vpgobj->vpg->vpg->header.itemNum-1)
    {
        uint32_t is_process = 0;
        lv_result_t res = lv_obj_send_event(obj, LV_EVENT_READY, &is_process);
        if (is_process)
        {
            return;
        }
        
    }

    vpg_load_frame(vpgobj->vpg, (uint8_t *)vpgobj->imgdsc.data);
    vpgobj->imgdsc.data_size = vpgobj->vpg->frame_size;
    lv_image_cache_drop(lv_image_get_src(obj));
    lv_obj_invalidate(obj);
}

static vpg_t *vpg_open(void *src){
    vpg_io_t const *io = NULL;
    void *io_ctx = NULL;
    if (src == NULL)
    {
        return NULL;
    }
    vpg_t *vpg = lv_malloc(sizeof(vpg_t));
    if (vpg == NULL)
    {
        return NULL;
    }
    
    if(lv_image_src_get_type(src) == LV_IMAGE_SRC_FILE) {
        /* 文件路径 */
        vpg_file_ctx_t *ctx = lv_malloc(sizeof(vpg_file_ctx_t));
        if(!ctx) { lv_free(vpg); return NULL; }
        if(lv_fs_open(&ctx->f, src, LV_FS_MODE_RD) != LV_FS_RES_OK) {
            lv_free(ctx); lv_free(vpg); return NULL;
        }
        io = &VPG_FILE_IO; io_ctx = ctx;
    }
    else if(lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
        /* 内存块 */
        const lv_image_dsc_t *mem = src;
        vpg_mem_ctx_t *ctx = lv_malloc(sizeof(vpg_mem_ctx_t));
        if(!ctx) { lv_free(vpg); return NULL; }
        ctx->base = (const uint8_t*)mem->data;
        ctx->size = mem->data_size;
        ctx->pos = 0;
        io = &VPG_MEM_IO; io_ctx = ctx;
    }
    else {
        lv_free(vpg);
        return NULL;
    }
    vpg->io = io;
    vpg->io_ctx = io_ctx;
    
    vpg_file_header_t header = {0};
    uint32_t total_size = 0;
    io->seek(io_ctx, 0, LV_FS_SEEK_END);
    io->tell(io_ctx, &total_size);
    io->seek(io_ctx, 0, LV_FS_SEEK_SET);
    if(io->read(io_ctx, &header, sizeof(header), NULL) != LV_FS_RES_OK || header.magic != 0xAABBCCDD) {
        io->close(io_ctx);
        lv_free(vpg);
        return NULL;
    }

    vpg->vpg = lv_malloc(sizeof(vpg_file_header_t) + header.itemNum * sizeof(vpg_item_header_t));
    if(!vpg->vpg) { io->close(io_ctx); lv_free(vpg); return NULL; }

    io->seek(io_ctx, 0, LV_FS_SEEK_SET);
    io->read(io_ctx, vpg->vpg, sizeof(vpg_file_header_t) + header.itemNum * sizeof(vpg_item_header_t), NULL);

    uint32_t max_size = 0;
    for (uint32_t i = 0; i < vpg->vpg->header.itemNum; i++)
    {
        // printf("vpg[%d] offset:%d size:%d\r\n", i, vpg->vpg->item[i].offset , vpg->vpg->item[i].size);
        if ((vpg->vpg->item[i].offset + vpg->vpg->item[i].size) > total_size)
        {
            // printf("!!!!!ERR: VPG FILE ERR");
            io->close(io_ctx);
            lv_free(vpg->vpg);
            lv_free(vpg);
            return NULL;
        }
        if (vpg->vpg->item[i].size > max_size)
        {
            max_size = vpg->vpg->item[i].size;
        }
        
    }

    vpg->index = vpg->vpg->header.itemNum-1;        
    vpg->frame_size = vpg->vpg->item[vpg->index].size;
    vpg->height = vpg->vpg->header.height;
    vpg->width = vpg->vpg->header.width;
    vpg->delay_ms = 1000/vpg->vpg->header.fps;
    vpg->frame = lv_malloc(max_size+1);
    if (vpg->frame == NULL)
    {
        io->close(io_ctx);
        lv_free(vpg->vpg);
        lv_free(vpg);
        return NULL;
    }
    io->seek(io_ctx, vpg->vpg->item[0].offset, LV_FS_SEEK_SET);
    io->read(io_ctx, vpg->frame, vpg->vpg->item[0].size, NULL);
    vpg->frame_size = vpg->vpg->item[0].size;
    return vpg;

}

static int vpg_get_frame(vpg_t *vpg){
    vpg->index = (vpg->index+1)%vpg->vpg->header.itemNum;
    return 1;
}

static void vpg_load_frame(vpg_t *vpg, uint8_t *frame){
    uint32_t read;
    vpg_file_t *file = vpg->vpg;
    vpg_item_header_t *it = &file->item[vpg->index];
    vpg->frame_size = it->size;
    vpg->io->seek(vpg->io_ctx, it->offset, LV_FS_SEEK_SET);
    vpg->io->read(vpg->io_ctx, frame, it->size, &read);
}

static void vpg_close(vpg_t *vpg){
    if(!vpg) return;
    if(vpg->frame) lv_free(vpg->frame);
    if(vpg->vpg) lv_free(vpg->vpg);
    if(vpg->io && vpg->io->close && vpg->io_ctx) vpg->io->close(vpg->io_ctx);
    lv_free(vpg);
}


/*==============================
 * IO 实现
 *=============================*/
static lv_fs_res_t vpg_file_read(void *ctx, void *buf, uint32_t size, uint32_t *out_read) {
    vpg_file_ctx_t *c = ctx;
    return lv_fs_read(&c->f, buf, size, out_read);
}
static lv_fs_res_t vpg_file_seek(void *ctx, uint32_t pos, uint8_t whence) {
    vpg_file_ctx_t *c = ctx;
    return lv_fs_seek(&c->f, pos, whence);
}
static lv_fs_res_t vpg_file_tell(void *ctx, uint32_t *pos) {
    vpg_file_ctx_t *c = ctx;
    return lv_fs_tell(&c->f, pos);
}
static void vpg_file_close(void *ctx) {
    vpg_file_ctx_t *c = ctx;
    lv_fs_close(&c->f);
    lv_free(c);
}

/* 内存实现 */
static lv_fs_res_t vpg_mem_read(void *ctx, void *buf, uint32_t size, uint32_t *out_read) {
    vpg_mem_ctx_t *c = ctx;
    if(c->pos >= c->size) { if(out_read) *out_read = 0; return LV_FS_RES_OK; }
    uint32_t remain = c->size - c->pos;
    uint32_t n = size < remain ? size : remain;
    if(n) lv_memcpy(buf, c->base + c->pos, n);
    c->pos += n;
    if(out_read) *out_read = n;
    return LV_FS_RES_OK;
}
static lv_fs_res_t vpg_mem_seek(void *ctx, uint32_t pos, uint8_t whence) {
    vpg_mem_ctx_t *c = ctx;
    uint32_t newpos = 0;
    if(whence == LV_FS_SEEK_SET) newpos = pos;
    else if(whence == LV_FS_SEEK_CUR) newpos = c->pos + pos;
    else if(whence == LV_FS_SEEK_END) newpos = (pos <= c->size) ? (c->size - pos) : c->size;
    else return LV_FS_RES_FS_ERR;
    if(newpos > c->size) newpos = c->size;
    c->pos = newpos;
    return LV_FS_RES_OK;
}
static lv_fs_res_t vpg_mem_tell(void *ctx, uint32_t *pos) {
    vpg_mem_ctx_t *c = ctx;
    if(pos) *pos = c->pos;
    return LV_FS_RES_OK;
}
static void vpg_mem_close(void *ctx) {
    lv_free(ctx);
}
