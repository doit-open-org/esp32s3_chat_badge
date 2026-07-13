#ifndef __SETTING_SLEEP_VIEW_H__
#define __SETTING_SLEEP_VIEW_H__

#include "lvgl.h"
#include <stdint.h>

typedef struct {
    lv_obj_t *root;
    lv_obj_t *set_target;
    lv_obj_t *set_curr;
    uint8_t is_act;
} setting_sleep_view_t;

// 视图管理函数
setting_sleep_view_t* setting_sleep_view_create(lv_obj_t *root);
void setting_sleep_view_delete(void);

// 自动关机管理函数（供其他模块调用）
void reset_auto_shutdown_timer(void);
uint32_t get_auto_shutdown_time(void);
void stop_auto_shutdown_timer(void);

#endif
