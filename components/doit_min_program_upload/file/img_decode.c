#include "img_decode.h"
#include "file_common.h"
#include "doit_decode.h"
#include "doit_ui.h"

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/param.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_system.h"

#include "esp_err.h"
#include "esp_vfs.h" // 追加
#include "esp_heap_caps.h"

#include "esp_lvgl_port.h" // 用 lvgl_port_lock/unlock
#include "lvgl.h"

static const char *TAG = "doit_img_file";

static lv_img_dsc_t doit_file_img_dsc_ = {0}; // 用于显示的图片

// 从 microSD 文件读取 JPEG，解码为 LVGL 可渲染的数据
jpeg_error_t doit_img_decode(const char *dir_name)
{
    //打印一下开始的时间
    ESP_LOGI(TAG, "doit_img_decode start");

    // ESP_LOGI(TAG, "doit_img_decode: %s", dir_name);
    
    // 先获取LVGL锁，确保原子性操作
    lvgl_port_lock(-1);
    
    // 释放旧JPG图片内存，防止内存泄漏
    if (doit_file_img_dsc_.data != NULL) {
        // ESP_LOGI(TAG, "Freeing old JPG image data: %p", doit_file_img_dsc_.data);
        
        // 不清空当前图片，避免闪烁，直接用新图片覆盖
        // lv_obj_t *img_obj = doit_ui_get_show_lv_obj();
        // if (img_obj) {
        //     lv_image_set_src(img_obj, NULL);  // 不要清空，避免闪烁
        //     lv_obj_invalidate(img_obj);       // 不要强制刷新空白
        // }
        
        heap_caps_free((void*)doit_file_img_dsc_.data);
        doit_file_img_dsc_.data = NULL;
        doit_file_img_dsc_.data_size = 0;
    }
    
    lvgl_port_unlock();
    
    // 减少延迟时间，避免过长的等待
    vTaskDelay(pdMS_TO_TICKS(20));
    
    uint8_t *out_buf = NULL;
    uint8_t *jpg_buf = NULL;  // 声明在函数开头
    int jpg_len = 0;          // 声明在函数开头
    jpeg_error_t ret = JPEG_ERR_OK;

    // 分配到栈里，避免堆分配
    jpeg_dec_io_t jpeg_io = {0};
    jpeg_dec_header_info_t out_info = {0};

    memset(&jpeg_io, 0, sizeof(jpeg_dec_io_t));
    memset(&out_info, 0, sizeof(jpeg_dec_header_info_t));

    FILE *f = fopen(dir_name, "rb");
    if (!f)
    {
        // ESP_LOGE(TAG, "fopen fail");
        ret = JPEG_ERR_FAIL;
        goto jpeg_dec_failed;
    }

    fseek(f, 0, SEEK_END);  // 将文件指针移动到文件末尾
    jpg_len = ftell(f); // 获取文件长度
    fseek(f, 0, SEEK_SET);  // 将文件指针重新定位到文件开头
    jpg_buf = (uint8_t *)heap_caps_malloc(jpg_len, MALLOC_CAP_SPIRAM);
    fread(jpg_buf, 1, jpg_len, f);
    fclose(f);

    // ESP_LOGI(TAG, "file size = %d", jpg_len);
    if (jpg_len < 2)
        goto jpeg_dec_failed;
    // ESP_LOG_BUFFER_HEX(TAG, jpg_buf, 16); // 看前 16 字节

    // 为I/O 控制结构体设置图片数据
    jpeg_io.inbuf = jpg_buf;
    jpeg_io.inbuf_len = jpg_len;

    // 解析 JPEG 图片的头部信息和图片数据。
    ret = jpeg_dec_parse_header(jpeg_dec, &jpeg_io, &out_info);
    if (ret != JPEG_ERR_OK)
    {
        goto jpeg_dec_failed;
    }

    // 分配输出数据缓冲区，并更新 inbuf 指针和 inbuf 的长度
    // 实现里，解析完 header 后把值带出来
    uint32_t pix_len = out_info.width * out_info.height * 3;

    if (config.output_type == JPEG_PIXEL_FORMAT_RGB565_LE || config.output_type == JPEG_PIXEL_FORMAT_RGB565_BE || config.output_type == JPEG_PIXEL_FORMAT_CbYCrY)
    {
        pix_len = out_info.width * out_info.height * 2;
    }
    else if (config.output_type == JPEG_PIXEL_FORMAT_RGB888)
    {
        pix_len = out_info.width * out_info.height * 3;
    }
    else
    {
        ret = JPEG_ERR_INVALID_PARAM;
        goto jpeg_dec_failed;
    }
    out_buf = heap_caps_aligned_alloc(16, // 对齐单位
                                      pix_len,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (out_buf == NULL)
    {
        ret = JPEG_ERR_NO_MEM;
        goto jpeg_dec_failed;
    }
    jpeg_io.outbuf = out_buf;

    // 解码图片
    ret = jpeg_dec_process(jpeg_dec, &jpeg_io);
    if (ret != JPEG_ERR_OK)
    {
        goto jpeg_dec_failed;
    }

    // ESP_LOGI(TAG, "img decode success %dx%d  %lu bytes", out_info.width, out_info.height, pix_len);
    doit_file_img_dsc_.header.cf = LV_COLOR_FORMAT_RGB565;
    doit_file_img_dsc_.header.w = out_info.width;
    doit_file_img_dsc_.header.h = out_info.height;
    doit_file_img_dsc_.data_size = pix_len;
    doit_file_img_dsc_.data = out_buf;

    lv_obj_t *img_obj = doit_ui_get_show_lv_obj();
    lvgl_port_lock(-1);
    lv_image_set_src(img_obj, &doit_file_img_dsc_);
    lv_obj_clear_flag(img_obj, LV_OBJ_FLAG_HIDDEN);  // 图像设置完成后再显示
    lv_obj_move_foreground(img_obj);  // 移动到最前面，覆盖所有UI
    lv_obj_invalidate(img_obj);
    lvgl_port_unlock();
    // ESP_LOGI(TAG, "PSD image shown and moved to foreground");

    // 释放JPG缓冲区，防止内存泄漏
    if (jpg_buf) {
        heap_caps_free(jpg_buf);
        jpg_buf = NULL;
        // ESP_LOGI(TAG, "JPG buffer freed, size: %d bytes", jpg_len);
    }

    // 强制内存整理和额外保护
    vTaskDelay(pdMS_TO_TICKS(20));  // 等待LVGL完成渲染
    ESP_LOGI(TAG, "doit_img_decode end");

    return ret;

    // Decoder deinitialize
jpeg_dec_failed:
    if (out_buf)
    {
        heap_caps_free(out_buf);
    }
    // 错误路径也要释放JPG缓冲区
    if (jpg_buf) {
        heap_caps_free(jpg_buf);
        ESP_LOGI(TAG, "JPG buffer freed in error path, size: %d bytes", jpg_len);
    }
    return ret;
   
   
}

// jpeg_error_t doit_img_decode2(lv_obj_t *img_obj, uint8_t *jpg_buf, int jpg_len)
// {
//     uint8_t *out_buf = NULL;
//     jpeg_error_t ret = JPEG_ERR_OK;
//     jpeg_dec_io_t *jpeg_io = NULL;
//     jpeg_dec_header_info_t *out_info = NULL;

//     // 配置解码器
//     jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
//     config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;
//     config.rotate = JPEG_ROTATE_0D;
//     // config.scale.width       = 0;
//     // config.scale.height      = 0;
//     // config.clipper.width     = 0;
//     // config.clipper.height    = 0;

//     // 创建jpeg解码器句柄
//     jpeg_dec_handle_t jpeg_dec = NULL;
//     ret = jpeg_dec_open(&config, &jpeg_dec);
//     if (ret != JPEG_ERR_OK)
//     {
//         goto jpeg_dec_failed;
//     }

//     // 创建JPEG 解码器 的 I/O 控制结构体,用于传入要解码的图片数据，喂给解码器
//     jpeg_io = calloc(1, sizeof(jpeg_dec_io_t));
//     if (jpeg_io == NULL)
//     {
//         ret = JPEG_ERR_NO_MEM;
//         goto jpeg_dec_failed;
//     }

//     // 创建JPEG 解码器 的 头信息结构体，用于获取图片的宽高信息
//     out_info = calloc(1, sizeof(jpeg_dec_header_info_t));
//     if (out_info == NULL)
//     {
//         ret = JPEG_ERR_NO_MEM;
//         goto jpeg_dec_failed;
//     }

//     // 为I/O 控制结构体设置图片数据
//     jpeg_io->inbuf = jpg_buf;
//     jpeg_io->inbuf_len = jpg_len;

//     // 解析 JPEG 图片的头部信息和图片数据。
//     ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
//     if (ret != JPEG_ERR_OK)
//     {
//         goto jpeg_dec_failed;
//     }

//     // 分配输出数据缓冲区，并更新 inbuf 指针和 inbuf 的长度
//     // 实现里，解析完 header 后把值带出来
//     uint32_t pix_len = out_info.width * out_info.height * 3;

//     if (config.output_type == JPEG_PIXEL_FORMAT_RGB565_LE || config.output_type == JPEG_PIXEL_FORMAT_RGB565_BE || config.output_type == JPEG_PIXEL_FORMAT_CbYCrY)
//     {
//         pix_len = out_info.width * out_info.height * 2;
//     }
//     else if (config.output_type == JPEG_PIXEL_FORMAT_RGB888)
//     {
//         pix_len = out_info.width * out_info.height * 3;
//     }
//     else
//     {
//         ret = JPEG_ERR_INVALID_PARAM;
//         goto jpeg_dec_failed;
//     }
//     out_buf = jpeg_calloc_align(pix_len, 16);
//     if (out_buf == NULL)
//     {
//         ret = JPEG_ERR_NO_MEM;
//         goto jpeg_dec_failed;
//     }
//     jpeg_io->outbuf = out_buf;

//     // 解码图片
//     ret = jpeg_dec_process(jpeg_dec, jpeg_io);
//     if (ret != JPEG_ERR_OK)
//     {
//         goto jpeg_dec_failed;
//     }

//     ESP_LOGI(TAG, "img decode success %dx%d  %lu bytes", out_info.width, out_info.height, pix_len);
//     doit_file_img_dsc_.header.cf = LV_COLOR_FORMAT_RGB565;
//     doit_file_img_dsc_.header.w = out_info.width;
//     doit_file_img_dsc_.header.h = out_info.height;
//     doit_file_img_dsc_.data_size = pix_len;
//     doit_file_img_dsc_.data = out_buf;

//     lvgl_port_lock(-1);
//     lv_image_set_src(img_obj, &doit_file_img_dsc_);
//     lv_obj_invalidate(img_obj);
//     lvgl_port_unlock();

//     // ESP_LOG_BUFFER_HEX(TAG, out_buf, pix_len);

//     jpeg_dec_close(jpeg_dec);
//     if (jpeg_io)
//     {
//         free(jpeg_io);
//     }
//     if (out_info)
//     {
//         free(out_info);
//     }
//     return ret;

//     // Decoder deinitialize
// jpeg_dec_failed:
//     jpeg_dec_close(jpeg_dec);
//     if (jpeg_io)
//     {
//         free(jpeg_io);
//     }
//     if (out_info)
//     {
//         free(out_info);
//     }
//     return ret;
// }
