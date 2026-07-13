#include "vpg_decode.h"
#include "file_common.h"
#include "doit_decode.h"
#include "doit_ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

const static char *TAG = "VPG_FILE";

#define VPG_BUFF_NUM 2
static bool buf_need_init[VPG_BUFF_NUM] = {true, true};

// vpg结构体
typedef struct lv_vpg_t
{
    FILE *fp;
    char spiff_path[64]; // 扩大以容纳SD卡路径
    bool is_vpg_player;
    lv_obj_t *img_obj;
    FileHeader fileHeader;
    ItemHeader *ItemHeader;
    uint16_t cur_frame_idx;
    uint8_t lv_buf_cur_idx;
    lv_image_dsc_t lv_buf[VPG_BUFF_NUM];
    uint8_t *psram_data;
    uint32_t psram_size;
} lv_vpg_t;

typedef struct
{
    uint8_t buf_idx;
} vpg_frame_t;

static lv_vpg_t *vpg = NULL;
static TaskHandle_t s_vpg_decode_task = NULL;
static TaskHandle_t s_vpg_show_task = NULL;
static QueueHandle_t jpg_decode_queue = NULL;
static volatile bool decode_idle = true;
static bool show_idle = true;
static volatile bool request_suspend = false; // ✅ 请求任务挂起的标志

// VPG资源保护
static SemaphoreHandle_t buf_free[VPG_BUFF_NUM] = {NULL};

/* 清理所有PSRAM资源 */
static void vpg_cleanup_resources(void)
{
    if (!vpg)
        return;

    if (vpg->psram_data)
    {
        heap_caps_free(vpg->psram_data);
        vpg->psram_data = NULL;
    }
    if (vpg->ItemHeader)
    {
        heap_caps_free(vpg->ItemHeader);
        vpg->ItemHeader = NULL;
    }
}

/* 完全销毁VPG系统，释放所有资源包括显示缓冲区 */
static void vpg_destroy_all_resources(void)
{
    if (!vpg)
        return;

    // 先清理文件相关资源
    vpg_cleanup_resources();

    // 释放显示缓冲区 - 只在完全退出时
    for (uint16_t i = 0; i < VPG_BUFF_NUM; ++i)
    {
        if (vpg->lv_buf[i].data)
        {
            heap_caps_free(vpg->lv_buf[i].data);
            vpg->lv_buf[i].data = NULL;
            vpg->lv_buf[i].data_size = 0;
        }
    }

    // 释放VPG结构体本身
    heap_caps_free(vpg);
    vpg = NULL;
}

/*清空队列并恢复信号量 */
static void vpg_clear_queue(void)
{
    if (!jpg_decode_queue)
        return;

    vpg_frame_t dummy;
    while (xQueueReceive(jpg_decode_queue, &dummy, 0) == pdTRUE)
    {
        xSemaphoreGive(buf_free[dummy.buf_idx]);
    }
}

/**
 * @brief 解码JPG数据
 */
static jpeg_error_t decode_jpg_from_mem(const uint8_t *jpg_data, const int jpg_len, lv_image_dsc_t *output_dsc)
{
    jpeg_error_t ret = JPEG_ERR_OK;
    jpeg_dec_io_t jpeg_io = {0};
    jpeg_dec_header_info_t out_info = {0};

    memset(&jpeg_io, 0, sizeof(jpeg_dec_io_t));
    memset(&out_info, 0, sizeof(jpeg_dec_header_info_t));

    jpeg_io.inbuf = jpg_data;
    jpeg_io.inbuf_len = jpg_len;

    ret = jpeg_dec_parse_header(jpeg_dec, &jpeg_io, &out_info);
    if (ret != JPEG_ERR_OK)
        return ret;

    output_dsc->header.w = out_info.width;
    output_dsc->header.h = out_info.height;

    if (config.output_type == JPEG_PIXEL_FORMAT_RGB565_LE ||
        config.output_type == JPEG_PIXEL_FORMAT_RGB565_BE ||
        config.output_type == JPEG_PIXEL_FORMAT_CbYCrY)
    {
        output_dsc->data_size = out_info.width * out_info.height * 2;
    }
    else if (config.output_type == JPEG_PIXEL_FORMAT_RGB888)
    {
        output_dsc->data_size = out_info.width * out_info.height * 3;
    }
    else
    {
        return JPEG_ERR_INVALID_PARAM;
    }

    output_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    jpeg_io.outbuf = output_dsc->data;
    ret = jpeg_dec_process(jpeg_dec, &jpeg_io);

    return ret;
}

/**
 * @brief 显示任务
 */
static void vpg_show_task(void *pvParameters)
{
    vpg_frame_t vpg_frame;
    static int frame_count = 0;
    int last_idx = -1; // 上一帧使用的缓冲索引
    ESP_LOGI(TAG, "VPG show task started     1");
    while (true)
    {
        show_idle = true;
        ESP_LOGI(TAG, "VPG show task running     2");

        // ✅ 检查是否需要挂起（在安全点主动挂起）
        if (request_suspend)
        {
            ESP_LOGI(TAG, "VPG show task suspending itself");
            vTaskSuspend(NULL); // 挂起自己
            // Resume后从这里继续
            ESP_LOGI(TAG, "VPG show task resumed");
            request_suspend = false;
        }

        if (!vpg || !vpg->is_vpg_player)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        show_idle = false;
        ESP_LOGI(TAG, "VPG show task active     3");
        if (xQueueReceive(jpg_decode_queue, &vpg_frame, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (!vpg || !vpg->is_vpg_player)
            {
                xSemaphoreGive(buf_free[vpg_frame.buf_idx]); // 释放缓冲区
                continue;
            }
            uint8_t idx = vpg_frame.buf_idx;
            static int show_count = 0;
            show_count++;

            // 每1000帧打印一次状态（减少日志）
            if (frame_count % 1000 == 0)
            {
                ESP_LOGI(TAG, "VPG playing: frame %d", frame_count);
            }
            ESP_LOGI(TAG, "VPG show task active     4");
            frame_count++;

            if (!vpg || !vpg->is_vpg_player)
            {
                continue;
            }

            // 检查img_obj是否为NULL
            if (!vpg->img_obj)
            {
                ESP_LOGW(TAG, "img_obj is NULL, skipping frame display");
                continue;
            }

            if (lvgl_port_lock(pdMS_TO_TICKS(100))) // 使用超时避免永久阻塞
            {
                // 再次检查img_obj（双重检查）
                if (!vpg->img_obj)
                {
                    lvgl_port_unlock();
                    continue;
                }

                // 检查LVGL对象是否有效
                if (!lv_obj_is_valid(vpg->img_obj))
                {
                    ESP_LOGW(TAG, "img_obj is invalid, skipping frame display");
                    lvgl_port_unlock();
                    continue;
                }

                ESP_LOGI(TAG, "VPG show task active     6");
                // 确保对象可见（每次都检查，避免对象被隐藏）
                if (lv_obj_has_flag(vpg->img_obj, LV_OBJ_FLAG_HIDDEN))
                {
                    lv_obj_clear_flag(vpg->img_obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_move_foreground(vpg->img_obj);
                    ESP_LOGI(TAG, "VPG object was hidden, now shown and moved to foreground");
                }

                lv_image_set_src(vpg->img_obj, &vpg->lv_buf[idx]);
                lv_obj_invalidate(vpg->img_obj);
                lvgl_port_unlock();
                ESP_LOGI(TAG, "VPG show task active     7");
            }
            else
            {
                ESP_LOGW(TAG, "Failed to acquire LVGL lock, skipping frame display");
            }

            // ✅ 释放"上一帧"的缓冲给解码线程复用（而不是当前帧）
            if (last_idx >= 0)
            {
                xSemaphoreGive(buf_free[last_idx]);
                ESP_LOGI(TAG, "Released buffer[%d]", last_idx);
            }
            last_idx = idx; // 记录当前帧索引

            ESP_LOGI(TAG, "VPG show task active     8");
        }

        // ✅ 在循环结束前立即设置idle状态
        // 这样即使在回到循环开始前被suspend，任务也已经是idle状态
        show_idle = true;
    }
}

/**
 * @brief 解码任务
 */
static void vpg_decode_task(void *pvParameters)
{
    while (true)
    {
        decode_idle = true;

        // ✅ 检查是否需要挂起（在安全点主动挂起）
        if (request_suspend)
        {
            ESP_LOGI(TAG, "VPG decode task suspending itself");
            vTaskSuspend(NULL); // 挂起自己
            // Resume后从这里继续
            ESP_LOGI(TAG, "VPG decode task resumed");
            request_suspend = false;
        }

        if (!vpg || !vpg->is_vpg_player)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        decode_idle = false;

        uint8_t idx = vpg->lv_buf_cur_idx;

        // 等待缓冲区可用
        if (xSemaphoreTake(buf_free[idx], pdMS_TO_TICKS(100)) != pdTRUE)
        {
            ESP_LOGW(TAG, "Decode task: timeout waiting for buf[%d], skipping frame", idx);
            goto next_frame;
        }

        if (!vpg->ItemHeader)
        {
            xSemaphoreGive(buf_free[idx]);
            goto next_frame;
        }

        uint32_t cur_fr_offset = vpg->ItemHeader[vpg->cur_frame_idx].offset;
        uint32_t cur_fr_size = vpg->ItemHeader[vpg->cur_frame_idx].size;

        fseek(vpg->fp, cur_fr_offset, SEEK_SET);
        size_t read_size = fread(vpg->psram_data, 1, cur_fr_size, vpg->fp);

        if (read_size != cur_fr_size)
        {
            ESP_LOGE(TAG, "Read fail frame %d, release buf", vpg->cur_frame_idx);
            xSemaphoreGive(buf_free[idx]);
            goto next_frame;
        }

        if (JPEG_ERR_OK == decode_jpg_from_mem(vpg->psram_data, cur_fr_size, &vpg->lv_buf[idx]))
        {
            vpg_frame_t msg = {.buf_idx = idx};
            // 修复：检查队列是否满，防止死锁
            if (xQueueSend(jpg_decode_queue, &msg, pdMS_TO_TICKS(5)) != pdTRUE)
            {
                ESP_LOGW(TAG, "Queue full, drop frame %d", vpg->cur_frame_idx);
                xSemaphoreGive(buf_free[idx]);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Decode fail skip frame %d", vpg->cur_frame_idx);
            xSemaphoreGive(buf_free[idx]);
        }

    next_frame:
        vpg->lv_buf_cur_idx = (idx + 1) % VPG_BUFF_NUM;
        if (++vpg->cur_frame_idx >= vpg->fileHeader.itemNum)
            vpg->cur_frame_idx = 0;

        vTaskDelay(pdMS_TO_TICKS(1000 / (vpg->fileHeader.fps + 5)));
    }
}

/**
 * @brief 停止播放
 */
void doit_vpg_player_stop(void)
{
    if (!vpg || !vpg->is_vpg_player)
    {
        ESP_LOGD(TAG, "VPG already stopped or not initialized");
        return;
    }

    ESP_LOGI(TAG, "Stopping VPG player, img_obj=%p", vpg->img_obj);

    // 1. 停止解码循环
    vpg->is_vpg_player = false;

    // 2. 清空img_obj指针，防止访问已删除的对象
    vpg->img_obj = NULL;

    // 3. 等待任务进入idle状态
    uint32_t wait_time = pdMS_TO_TICKS(500);
    TickType_t start = xTaskGetTickCount();

    while ((!decode_idle || !show_idle) && (xTaskGetTickCount() - start) < wait_time)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!decode_idle || !show_idle)
    {
        ESP_LOGW(TAG, "Tasks not idle (decode=%d, show=%d), but continuing",
                 decode_idle, show_idle);
    }
    else
    {
        ESP_LOGI(TAG, "Tasks entered idle state successfully");
    }

    // 4. ✅ 请求任务主动挂起自己（而不是强制挂起）
    request_suspend = true;

    // 5. 等待任务完成挂起（检查任务状态）
    wait_time = pdMS_TO_TICKS(500);
    start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < wait_time)
    {
        // 检查任务是否已经挂起
        eTaskState decode_state = eTaskGetState(s_vpg_decode_task);
        eTaskState show_state = eTaskGetState(s_vpg_show_task);

        if (decode_state == eSuspended && show_state == eSuspended)
        {
            ESP_LOGI(TAG, "Both tasks suspended successfully");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    request_suspend = false; // 清除标志

    // 3. 清空队列
    vpg_clear_queue();

    // 4. 关闭文件
    if (vpg->fp)
    {
        fclose(vpg->fp);
        vpg->fp = NULL;
    }

    // 5. 释放所有信号量并标记需要初始化
    for (int i = 0; i < VPG_BUFF_NUM; ++i)
    {
        // 确保信号量可用
        xSemaphoreGive(buf_free[i]);
        buf_need_init[i] = true;
        // ESP_LOGI(TAG, "VPG buf[%d] released", i);
    }

    // 6. 清理PSRAM资源（依赖前面的等待机制）
    vpg_cleanup_resources();

    ESP_LOGI(TAG, "VPG stopped, PSRAM free: %d KB",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

/**
 * @brief 完全清理VPG系统，释放所有资源
 */
void doit_vpg_player_cleanup(void)
{
    // 先停止播放
    doit_vpg_player_stop();

    // 然后完全销毁所有资源
    vpg_destroy_all_resources();

    ESP_LOGI(TAG, "VPG system completely destroyed, PSRAM free: %d KB",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

/**
 * @brief 启动播放
 */
void doit_vpg_player_start(const char *dir_name)
{
    if (!vpg)
    {
        // 首次运行时初始化
        vpg = heap_caps_calloc(1, sizeof(lv_vpg_t), MALLOC_CAP_SPIRAM);
        if (!vpg)
        {
            ESP_LOGE(TAG, "vpg malloc fail");
            return;
        }

        jpg_decode_queue = xQueueCreate(1, sizeof(vpg_frame_t));
        vpg->img_obj = doit_ui_get_show_lv_obj();

        // 分配显示缓冲区
        uint16_t width = 0, height = 0;
        doit_get_ui_screen_size(&width, &height);
        for (uint16_t i = 0; i < VPG_BUFF_NUM; ++i)
        {
            vpg->lv_buf[i].data = (uint8_t *)heap_caps_aligned_alloc(
                16, width * height * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!vpg->lv_buf[i].data)
            {
                ESP_LOGE(TAG, "PSRAM malloc failed for lv_buf[%d]", i);
                // 显示缓冲区分配失败，停止VPG播放器
                vpg->is_vpg_player = false;
                return;
            }
            // 初始化图像描述符的header字段
            vpg->lv_buf[i].header.cf = LV_COLOR_FORMAT_RGB565;
            vpg->lv_buf[i].header.w = width;
            vpg->lv_buf[i].header.h = height;
            vpg->lv_buf[i].data_size = width * height * 2;
        }

        // 创建信号量
        for (int i = 0; i < VPG_BUFF_NUM; ++i)
        {
            buf_free[i] = xSemaphoreCreateBinary();
            buf_need_init[i] = true;
        }

        // 创建任务
        BaseType_t ret = xTaskCreatePinnedToCore(
            vpg_show_task, "vpg_show_task", 3072, NULL, 6, &s_vpg_show_task, 1);
        if (ret != pdPASS)
        {
            ESP_LOGE(TAG, "show task create fail");
            // 任务创建失败，清理已分配的资源
            vpg_cleanup_resources();
            heap_caps_free(vpg);
            vpg = NULL;
            return;
        }
        vTaskSuspend(s_vpg_show_task);

        ret = xTaskCreatePinnedToCore(
            vpg_decode_task, "vpg_decode_task", 1728, NULL, 6, &s_vpg_decode_task, 1);
        if (ret != pdPASS)
        {
            ESP_LOGE(TAG, "decode task create fail");
            // 解码任务创建失败，清理已创建的显示任务和资源
            vTaskDelete(s_vpg_show_task);
            s_vpg_show_task = NULL;
            vpg_cleanup_resources();
            heap_caps_free(vpg);
            vpg = NULL;
            return;
        }
        vTaskSuspend(s_vpg_decode_task);

        // ESP_LOGI(TAG, "VPG system initialized");
    }

    /* 更新显示对象 */
    lv_obj_t *new_img_obj = doit_ui_get_show_lv_obj();
    if (new_img_obj != vpg->img_obj)
    {
        // ESP_LOGW(TAG, "Display object changed! Updating from %p to %p", vpg->img_obj, new_img_obj);
        vpg->img_obj = new_img_obj;
    }
    // 检查显示对象有效性
    if (!vpg->img_obj)
    {
        // ESP_LOGE(TAG, "Display object is NULL! VPG cannot display");
        return;
    }

    /* 初始化播放参数 */
    vpg->is_vpg_player = true;
    vpg->cur_frame_idx = 0;
    strncpy(vpg->spiff_path, dir_name, 63);
    vpg->spiff_path[63] = '\0';
    vpg->lv_buf_cur_idx = 0;

    /* 清空队列 */
    vpg_clear_queue();

    /*重置信号量*/
    for (int i = 0; i < VPG_BUFF_NUM; ++i)
    {
        if (buf_need_init[i])
        {
            // 清空信号量（可能有多个）
            while (xSemaphoreTake(buf_free[i], 0) == pdTRUE)
            {
                // 继续取，直到空
            }
            // 给一个信号量
            xSemaphoreGive(buf_free[i]);
            buf_need_init[i] = false;
        }
    }

    /* 打开文件 */
    vpg->fp = fopen(vpg->spiff_path, "rb");
    if (!vpg->fp)
    {
        // ESP_LOGE(TAG, "Open failed: %s", vpg->spiff_path);
        // 文件打开失败，停止VPG播放器避免任务访问无效资源
        vpg->is_vpg_player = false;
        return;
    }

    // 读取文件头
    if (fread(&vpg->fileHeader, sizeof(FileHeader), 1, vpg->fp) != 1 ||
        vpg->fileHeader.magic != 0xAABBCCDD)
    {
        // ESP_LOGE(TAG, "Invalid VPG file");
        fclose(vpg->fp);
        vpg->fp = NULL;
        // 文件格式无效，停止VPG播放器
        vpg->is_vpg_player = false;
        return;
    }

    // 分配并读取索引表
    size_t idx_bytes = vpg->fileHeader.itemNum * sizeof(ItemHeader);
    vpg->ItemHeader = heap_caps_malloc(idx_bytes, MALLOC_CAP_SPIRAM);
    if (!vpg->ItemHeader)
    {
        // ESP_LOGE(TAG, "ItemHeader malloc fail");
        fclose(vpg->fp);
        vpg->fp = NULL;
        // 内存分配失败，停止VPG播放器
        vpg->is_vpg_player = false;
        return;
    }
    fread(vpg->ItemHeader, idx_bytes, 1, vpg->fp);

    // 计算最大帧大小
    uint32_t max_frame_size = 0;
    for (uint16_t i = 0; i < vpg->fileHeader.itemNum; i++)
    {
        if (vpg->ItemHeader[i].size > max_frame_size)
        {
            max_frame_size = vpg->ItemHeader[i].size;
        }
    }
    // ESP_LOGI(TAG, "Max frame size: %lu bytes", max_frame_size);

    // 分配帧缓冲区
    vpg->psram_data = heap_caps_malloc(max_frame_size, MALLOC_CAP_SPIRAM);
    if (!vpg->psram_data)
    {
        // ESP_LOGE(TAG, "PSRAM malloc fail");
        fclose(vpg->fp);
        vpg->fp = NULL;
        // 帧缓冲区分配失败，停止VPG播放器
        vpg->is_vpg_player = false;
        return;
    }
    vpg->psram_size = max_frame_size;

    // 启动任务
    vTaskResume(s_vpg_show_task);
    vTaskResume(s_vpg_decode_task);

    // ESP_LOGI(TAG, "VPG started, PSRAM free: %d KB",
    //          heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}