#include "vw_alarm_ring.h"
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"
#include "cl_ui.h"
#include "ui_tool.h"
static alarm_ring_view_t vw;
extern void xiaozhi_alarm_stop(void);
LV_FONT_DECLARE(font_puhui_20_4)
LV_FONT_DECLARE(font_puhui_ascii_80_4)
LV_IMG_DECLARE(icon_alarm_64)

static void _on_btn_cb(lv_event_t *e)
{
    if (!vw.is_act)
    {
        return;
    }

    cl_button_t *btn = (cl_button_t *)lv_event_get_param(e);
    if (!btn)
        return;

    switch (btn->id)
    {
    case CL_UI_KEY_MODE:
        if (btn->event == CL_BTN_CLICK || btn->event == CL_BTN_DOUBLE_CLICK)
        {
            xiaozhi_alarm_stop();
            page_change("home");
        }
        break;
    default:
        break;
    }
}

// 点击停止按钮事件
static void exit_click_cb(lv_event_t *e)
{
    xiaozhi_alarm_stop();
    page_change("home");
}

static void clock_time_cb(lv_timer_t *timer)
{
    if (!vw.is_act)
    {
        return;
    }

    // 获取当前时间
    cl_time_t time = ui_tool_get_time();
    lv_label_set_text_fmt(vw.time_label, "%02d:%02d", time.hour, time.min);
}

alarm_ring_view_t* alarm_ring_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);

    // 在 root 下创建一个容器，并设置背景色
    lv_obj_t *container = lv_obj_create(root);
    lv_obj_remove_style_all(container);  // 清除默认样式
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);  // 设置容器大小为屏幕大小
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);              // 设置背景不透明度为不透明
    lv_obj_set_style_border_width(container, 0, 0);                   // 清除边框
    lv_obj_set_style_pad_all(container, 0, 0);                         // 清除内边距
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(container);    
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);  // 设置背景色为黑色

    cl_time_t time = ui_tool_get_time();
    lv_obj_t *time_label = lv_label_create(container);
    vw.time_label = time_label;
    lv_obj_set_style_text_color(time_label,lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_font(time_label, &font_puhui_ascii_80_4, 0);
    lv_label_set_text_fmt(time_label, "%02d:%02d", time.hour, time.min);
    lv_obj_center(time_label);
    
    lv_obj_t *alarm_label = lv_img_create(container);
    lv_img_set_src(alarm_label, &icon_alarm_64);
    lv_obj_align_to(alarm_label, time_label, LV_ALIGN_OUT_TOP_MID, 0, -20);

    lv_obj_t *btn_exit = lv_button_create(container);
    lv_obj_set_size(btn_exit, 160, 40);
    lv_obj_set_style_radius(btn_exit, 40, 0);
    lv_obj_set_style_bg_color(btn_exit, lv_color_hex(0Xff5353), LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(btn_exit, 0, 0);
    lv_obj_set_style_shadow_opa(btn_exit, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_ofs_x(btn_exit, 0, 0);
    lv_obj_set_style_shadow_ofs_y(btn_exit, 0, 0);   
    lv_obj_align(btn_exit, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_t *exit_label = lv_label_create(btn_exit);
    lv_obj_set_style_text_font(exit_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(exit_label, lv_color_white(), 0);
    lv_label_set_text(exit_label, "停止");
    lv_obj_center(exit_label);

    lv_obj_add_event_cb(btn_exit, exit_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    lv_label_set_text_fmt(time_label, "%02d:%02d", time.hour, time.min);
    lv_group_focus_obj(btn_exit);
    
    extern lv_indev_t *indev_usr_btn;
    static uint8_t first_flag = 1;
    if (first_flag)
    {
        first_flag = 0;
        lv_timer_create(clock_time_cb ,1000, NULL);
    }
    
    return &vw;
}

void alarm_ring_view_delete(void)
{

}

void alarm_ring_view_appear_anim_start(bool reverse)
{
    // 如果需要反向动画，可以在这里实现
}
