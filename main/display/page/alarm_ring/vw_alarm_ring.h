#ifndef __alarm_ring_VIEW_H__
#define __alarm_ring_VIEW_H__

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

typedef struct 
{
    uint8_t index;
    uint8_t hour;
    uint8_t min;
    uint8_t week_mask;
}alarm_ring_info_t;

typedef struct 
{
    lv_obj_t *status_bar;
    lv_obj_t *time_label;

    lv_obj_t *list_cont;
    lv_obj_t *btn_add;

    uint32_t ring_time;
    alarm_ring_info_t alarm_ring_info[10];



    int is_act;
}alarm_ring_view_t;




alarm_ring_view_t* alarm_ring_view_create(lv_obj_t *root);
void alarm_ring_view_delete(void);
void alarm_ring_view_appear_anim_start(bool reverse);

#endif