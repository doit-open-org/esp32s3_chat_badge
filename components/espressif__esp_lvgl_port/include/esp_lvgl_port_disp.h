/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LVGL port display
 */

#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "driver/gpio.h"
#if LVGL_VERSION_MAJOR == 8
#include "esp_lvgl_port_compatibility.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rotation configuration
 */
typedef struct {
    bool swap_xy;  /*!< LCD Screen swapped X and Y (in esp_lcd driver) */
    bool mirror_x; /*!< LCD Screen mirrored X (in esp_lcd driver) */
    bool mirror_y; /*!< LCD Screen mirrored Y (in esp_lcd driver) */
} lvgl_port_rotation_cfg_t;

/**
 * @brief Configuration display structure
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;    /*!< LCD panel IO handle */
    esp_lcd_panel_handle_t panel_handle;    /*!< LCD panel handle */
    esp_lcd_panel_handle_t control_handle;  /*!< LCD panel control handle */
    bool enable_tearing_effect;             /* 是否开启撕裂处理。刷新的时机需要匹配好，否则还是会存在撕裂。
                                                te的非扫描周期和扫描显示周期也可以通过调整vfp和vbp寄存器调整。 
                                                要知道时机是否正确，可以抓取cs或clk引脚和te引脚，cs引脚低电平代表传输，
                                                clk可以看出传的数据的时序（前一部分是命令，后面的是数据，还可以看到数据分成了几部分传递），
                                                te高电平时是非扫描周期（若完整一帧在该阶段写完，则无撕裂。若写不完，看实际写入及屏幕扫描的时机），低电平是扫描显示周期。
                                                要注意屏幕的0x36寄存器中的MX,MY其实只调整地址的顺序，而不改变实际扫描顺序，还需要调整ML和MH；
                                                或者调整SS和GS,SS=MX+MH，GS=MY+ML；
                                                若没有ML,MH，也没有GS,SS，可以看有没有其他寄存器可以调整，比如ST7789的0xE4(GATECTRL) */
    gpio_num_t te_gpio;                     /* 屏幕撕裂处理信号引脚。若为-1或不开启撕裂处理时，该配置项无效 */
    int tear_effect_level_edge;             /* 撕裂处理等待的时机，1为在每次上升沿后发送数据，0为在每次下降沿后发送数据。若不开启撕裂处理，该配置项无效 */
    uint32_t tearing_timeout_ms;            /* 撕裂信号的超时时间，单位毫秒。若不开启撕裂处理，该配置项无效 */
    uint32_t    buffer_size;        /*!< Size of the buffer for the screen in pixels */
    bool        double_buffer;      /*!< True, if should be allocated two buffers */
    uint32_t    trans_size;         /*!< Allocated buffer will be in SRAM to move framebuf (optional) */

    uint32_t    hres;           /*!< LCD display horizontal resolution */
    uint32_t    vres;           /*!< LCD display vertical resolution */

    bool        monochrome;     /*!< True, if display is monochrome and using 1bit for 1px */

    lvgl_port_rotation_cfg_t rotation;      /*!< Default values of the screen rotation (Only HW state. Not supported for default SW rotation!) */
#if LVGL_VERSION_MAJOR >= 9
    lv_color_format_t        color_format;  /*!< The color format of the display */
#endif
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
        unsigned int sw_rotate: 1;   /*!< Use software rotation (slower) or PPA if available */
#if LVGL_VERSION_MAJOR >= 9
        unsigned int swap_bytes: 1;  /*!< Swap bytes in RGB565 (16-bit) color format before send to LCD driver */
#endif
        unsigned int full_refresh: 1;/*!< 1: Always make the whole screen redrawn */
        unsigned int direct_mode: 1; /*!< 1: Use screen-sized buffers and draw to absolute coordinates */
    } flags;
} lvgl_port_display_cfg_t;

/**
 * @brief Configuration RGB display structure
 */
typedef struct {
    struct {
        unsigned int bb_mode: 1;        /*!< 1: Use bounce buffer mode */
        unsigned int avoid_tearing: 1;  /*!< 1: Use internal RGB buffers as a LVGL draw buffers to avoid tearing effect, enabling this option requires over two LCD buffers and may reduce the frame rate */
    } flags;
} lvgl_port_display_rgb_cfg_t;

/**
 * @brief Configuration MIPI-DSI display structure
 */
typedef struct {
    struct {
        unsigned int avoid_tearing: 1;  /*!< 1: Use internal MIPI-DSI buffers as a LVGL draw buffers to avoid tearing effect, enabling this option requires over two LCD buffers and may reduce the frame rate */
    } flags;
} lvgl_port_display_dsi_cfg_t;

/**
 * @brief Add I2C/SPI/I8080 display handling to LVGL
 *
 * @note Allocated memory in this function is not free in deinit. You must call lvgl_port_remove_disp for free all memory!
 *
 * @param disp_cfg Display configuration structure
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *disp_cfg);

/**
 * @brief Add MIPI-DSI display handling to LVGL
 *
 * @note Allocated memory in this function is not free in deinit. You must call lvgl_port_remove_disp for free all memory!
 *
 * @param disp_cfg Display configuration structure
 * @param dsi_cfg MIPI-DSI display specific configuration structure
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *lvgl_port_add_disp_dsi(const lvgl_port_display_cfg_t *disp_cfg, const lvgl_port_display_dsi_cfg_t *dsi_cfg);

/**
 * @brief Add RGB display handling to LVGL
 *
 * @note Allocated memory in this function is not free in deinit. You must call lvgl_port_remove_disp for free all memory!
 *
 * @param disp_cfg Display configuration structure
 * @param rgb_cfg RGB display specific configuration structure
 * @return Pointer to LVGL display or NULL when error occurred
 */
lv_display_t *lvgl_port_add_disp_rgb(const lvgl_port_display_cfg_t *disp_cfg, const lvgl_port_display_rgb_cfg_t *rgb_cfg);

/**
 * @brief Remove display handling from LVGL
 *
 * @note Free all memory used for this display.
 *
 * @return
 *      - ESP_OK                    on success
 */
esp_err_t lvgl_port_remove_disp(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
