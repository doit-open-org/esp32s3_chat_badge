#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include "esp_err.h"
#include "driver/sdmmc_host.h"

#define ZW_SDCARD_MOUNTPOINT "/sdcard"
#define ZW_SDCARD_MAX_OPEN_FILES 3
#define ZW_SDCARD_ALLOCATE_UNIT_SIZE 0

    void zw_sdcard_usb_msc_enable();
    void zw_sdcard_usb_msc_disable();
    bool zw_sdcard_is_usb_mode(void); // 查询当前是否为U盘模式

    // USB检测相关
    void zw_sdcard_usb_detect_init(void);                           // 初始化USB检测
    bool zw_sdcard_is_usb_connected(void);                          // 检查USB是否连接
    void zw_sdcard_set_usb_unplug_callback(void (*callback)(void)); // 设置USB拔出回调

    esp_err_t zw_sdcard_init();
    void zw_sdcard_deinit();

#ifdef __cplusplus
}
#endif