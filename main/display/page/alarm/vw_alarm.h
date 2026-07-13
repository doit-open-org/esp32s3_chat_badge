#ifndef __alarm_VIEW_H__
#define __alarm_VIEW_H__

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "services/alarm/xiaozhi_alarm.h"

typedef enum{

    ALARM_MODE_IDLE = 0, // 闲置状态
    ALARM_MODE_ADD,      // 添加闹钟状态
    ALARM_MODE_EDIT,     // 编辑闹钟状态
}alarm_operation_mode_t;

typedef struct 
{

    lv_obj_t *cont_alarm_list;
    lv_obj_t *cont_alarm_time_set;
    lv_obj_t *cont_alarm_week_set;
    lv_obj_t *cont_alarm_info;
    lv_obj_t *list_cont;
    lv_obj_t *btn_add;

    lv_obj_t *label_alarm_info_time;
    lv_obj_t *label_alarm_info_week;
    uint8_t curr_edit_index; // 当前编辑的闹钟索引

    lv_obj_t *btn_time_set;

    lv_obj_t *roller_hour;
    lv_obj_t *roller_min;

    lv_obj_t *btn_week[7]; // 星期按钮

    alarm_operation_mode_t curr_mode;
    xz_alarm_info_t *alarm_info;
    uint8_t alarm_num;
    int is_act;
}alarm_view_t;




alarm_view_t* alarm_view_create(lv_obj_t *root);
void alarm_view_delete(void);
void alarm_view_appear_anim_start(bool reverse);

#endif