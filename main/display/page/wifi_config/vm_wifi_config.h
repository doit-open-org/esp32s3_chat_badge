#ifndef WIFI_CONFIG_VIEW_H
#define WIFI_CONFIG_VIEW_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *main_label;
    lv_obj_t *routine_img;
    lv_obj_t *device_code_label;
    lv_obj_t *ap_hint_label;
    lv_obj_t *ap_title_label;
    lv_timer_t *check_timer;   // 用于检查 WiFi 连接状态的定时器
    bool is_active;            // 页面是否激活
    bool show_device_code;
    bool show_ap_hint;
} wifi_config_view_t;

wifi_config_view_t* wifi_config_view_create(lv_obj_t *root);
void wifi_config_view_delete(void);
void wifi_config_view_set_active(bool active);
void wifi_config_view_show_device_code(bool show);
void wifi_config_view_set_ap_hint(const char *hint);

#ifdef __cplusplus
}
#endif

#endif
