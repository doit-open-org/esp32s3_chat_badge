/**
 * @file lv_gif_private.h
 *
 */

#ifndef LV_VPG_PRIVATE_H
#define LV_VPG_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef SIMULATOR
#include "src/widgets/image/lv_image_private.h"
#else
#include "widgets/image/lv_image_private.h"
#endif
#include "lv_vpg.h"

#if LV_USE_GIF

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_fs_res_t (*read)(void *ctx, void *buf, uint32_t size, uint32_t *out_read);
    lv_fs_res_t (*seek)(void *ctx, uint32_t pos, uint8_t whence);
    lv_fs_res_t (*tell)(void *ctx, uint32_t *pos);
    void        (*close)(void *ctx);
} vpg_io_t;

typedef struct {
    lv_fs_file_t f;
} vpg_file_ctx_t;

typedef struct {
    const uint8_t *base;
    uint32_t size;
    uint32_t pos;
} vpg_mem_ctx_t;

#pragma pack(push, 1) // 设置结构体为1字节对齐
typedef struct {
    uint32_t offset; // 4 bytes offset
    uint32_t size;  // 2 bytes size
} vpg_item_header_t;

typedef struct {
    uint32_t magic; // 4 bytes offset
    uint32_t size;  // 2 bytes size
    uint32_t itemNum;  // 2 bytes size
    uint32_t fps:8;  // 2 bytes size
    uint32_t height:12;  // 2 bytes size
    uint32_t width:12;  // 2 bytes size
} vpg_file_header_t;

typedef struct
{
    vpg_file_header_t header;
    vpg_item_header_t item[0];
}vpg_file_t;


typedef struct
{
    // lv_fs_file_t f;
    uint8_t *frame;
    uint32_t frame_size;
    uint16_t delay_ms;
    uint16_t index;
    uint32_t width:16;
    uint32_t height:16;
    vpg_file_t *vpg;
    const vpg_io_t *io;
    void *io_ctx;
}vpg_t;
#pragma pack(pop) // 恢复对齐方式

struct lv_vpg_t {
    lv_image_t img;
    vpg_t *vpg;
    lv_timer_t * timer;
    lv_image_dsc_t imgdsc;
    uint32_t last_call;
};


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_GIF */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GIF_PRIVATE_H*/
