#include "lvgl.h"
#include "cl_ui.h"
#include "vw_app_list.h"
#include "assert/language.h"
#include <string.h>
#include "esp_log.h"
#include "ui_tool.h"

#define SCREEN_W 360
#define SCREEN_H 360
#define LT_AI "AI"

LV_IMG_DECLARE(bat_empty)
LV_FONT_DECLARE(font_puhui_20_4)
LV_IMG_DECLARE(icon_ai_32)
LV_IMG_DECLARE(icon_ai_64)
LV_IMG_DECLARE(icon_alarm_32)
LV_IMG_DECLARE(icon_alarm_64)
LV_IMG_DECLARE(icon_video_32)
LV_IMG_DECLARE(icon_anim_64)
LV_IMG_DECLARE(icon_setting_32)
LV_IMG_DECLARE(icon_setting_64)
LV_IMG_DECLARE(icon_power_32)
LV_IMG_DECLARE(icon_bright_24)
LV_IMG_DECLARE(icon_vol_24)

#define TAG "vw_app_list"

static lv_font_t *s_main_font = NULL;
static app_list_view_t vw = {0};

static void _on_app_click(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const char *name = (const char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED && name != NULL)
    {
        if (strcmp(name, LT_ALARM) == 0)
        {
            page_change("alarm");
        }
        else if (strcmp(name, LT_SETTING) == 0)
        {
            page_change("setting");
        }
        else if (strcmp(name, LT_AI) == 0)
        {
            page_change("ai");
        }
        else if (strcmp(name, LT_ANIM) == 0)
        {
            page_change("animation");
        }
    }
}

static lv_obj_t *_create_circle_btn(lv_obj_t *parent, const char *user_data, const void *src, lv_color_t color, int size, lv_event_cb_t event_cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size + 35);
    lv_obj_set_style_radius(btn, size / 2, 0);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    // 不显示文字标签，注释掉这部分
    // if (user_data)
    // {
    //     lv_obj_t *label = lv_label_create(btn);
    //     lv_obj_set_style_text_font(label, s_main_font, 0);
    //     lv_obj_set_style_text_color(label, lv_color_hex3(0xfff), 0);
    //     lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -45);
    //     lv_label_set_text(label, user_data);
    // }

    if (src)
    {
        lv_obj_t *img = lv_image_create(btn);
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);
        lv_image_set_src(img, src);
    }

    if (event_cb)
    {
        lv_obj_add_event_cb(btn, event_cb, LV_EVENT_ALL, (void *)user_data);
    }

    return btn;
}

static void _on_btn_cb(lv_event_t *e)
{
    if (!vw.is_act)
        return;

    cl_button_t *btn = (cl_button_t *)lv_event_get_param(e);
    if (!btn)
        return;

    switch (btn->id)
    {
    case CL_UI_KEY_MODE:
        // 模式键单击：返回主页
        if (btn->event == CL_BTN_CLICK)
        {
            page_change("home");
        }
        break;

    default:
        break;
    }
}
static void _on_bright_change(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        int value = lv_slider_get_value(slider);

        ui_tool_set_brightness((uint8_t)value);
        }
}

static void _on_vol_change(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        ui_tool_set_volume(value);
    }
}

app_list_view_t *app_list_view_create(lv_obj_t *root)
{
    if (s_main_font == NULL)
    {
        s_main_font = (lv_font_t *)&font_puhui_20_4;
    }

    // 设置根对象样式
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    // lv_obj_set_style_bg_color(root,lv_color_white(),0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    // 电量指示器
    lv_obj_t *bat_img = lv_image_create(root);
    lv_image_set_src(bat_img, &bat_empty);
    lv_obj_align(bat_img, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *bat_bar = lv_bar_create(bat_img);
    lv_obj_set_size(bat_bar, 14, 8);
    lv_obj_align(bat_bar, LV_ALIGN_LEFT_MID, 7, 0);
    lv_obj_set_style_radius(bat_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bat_bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bat_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bat_bar, lv_color_black(), LV_PART_MAIN);

    // 获取充电状态并设置颜色
    uint8_t state = ui_tool_get_charging_state();
    lv_obj_set_style_bg_color(bat_bar,
                              state ? lv_color_hex(0x3ee923) : lv_color_hex(0xffffff),
                              LV_PART_INDICATOR);

    // 获取电量百分比并设置进度条
    uint8_t percent = ui_tool_get_battery_percent();
    if (percent > 90)
        percent = 100;
    lv_bar_set_value(bat_bar, percent, LV_ANIM_ON);

    // 创建应用按钮 (2x2 布局)
    lv_obj_t *btn_anim = _create_circle_btn(root, LT_ANIM, &icon_anim_64, lv_color_hex(0x0), 64, _on_app_click);
    lv_obj_t *btn_alarm = _create_circle_btn(root, LT_ALARM, &icon_alarm_64, lv_color_hex(0x0), 64, _on_app_click);
    lv_obj_t *btn_setting = _create_circle_btn(root, LT_SETTING, &icon_setting_64, lv_color_hex(0x0), 64, _on_app_click);
    lv_obj_t *btn_ai = _create_circle_btn(root, LT_AI, &icon_ai_64, lv_color_hex(0x0), 64, _on_app_click);

    lv_obj_align(btn_anim, LV_ALIGN_TOP_MID, -60, 72);
    lv_obj_align(btn_alarm, LV_ALIGN_TOP_MID, 60, 72);
    lv_obj_align(btn_setting, LV_ALIGN_TOP_MID, -60, 170);
    lv_obj_align(btn_ai, LV_ALIGN_TOP_MID, 60, 170);

    // 亮度滑块
    lv_obj_t *bright_slider = lv_slider_create(root);
    lv_obj_set_style_bg_opa(bright_slider, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(bright_slider, 20, LV_PART_MAIN);
    lv_obj_set_style_radius(bright_slider, 0, LV_PART_INDICATOR);
    lv_obj_set_size(bright_slider, 200, 40);
    lv_obj_align(bright_slider, LV_ALIGN_TOP_MID, 0, 250);
    lv_obj_set_style_bg_color(bright_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bright_slider, lv_color_hex(0x1E90FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bright_slider, 255, LV_PART_MAIN);

    lv_obj_t *bright_icon = lv_image_create(bright_slider);
    lv_image_set_src(bright_icon, &icon_bright_24);
    lv_obj_align(bright_icon, LV_ALIGN_LEFT_MID, 16, 0);

    uint8_t current_brightness = ui_tool_get_brightness();
    lv_slider_set_value(bright_slider, current_brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(bright_slider, _on_bright_change, LV_EVENT_VALUE_CHANGED, NULL);

    // 音量滑块
    lv_obj_t *vol_slider = lv_slider_create(root);
    lv_obj_set_style_bg_opa(vol_slider, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(vol_slider, 20, LV_PART_MAIN);
    lv_obj_set_style_radius(vol_slider, 0, LV_PART_INDICATOR);
    lv_obj_set_size(vol_slider, 200, 40);
    lv_obj_align_to(vol_slider, bright_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(vol_slider, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0x1E90FF), LV_PART_INDICATOR);

    lv_obj_t *vol_icon = lv_image_create(vol_slider);
    lv_image_set_src(vol_icon, &icon_vol_24);
    lv_obj_align(vol_icon, LV_ALIGN_LEFT_MID, 16, 0);

    int current_volume = ui_tool_get_volume();
    lv_slider_set_value(vol_slider, current_volume, LV_ANIM_OFF);
    lv_obj_add_event_cb(vol_slider, _on_vol_change, LV_EVENT_VALUE_CHANGED, NULL);

    // 注册按键事件
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);

    vw.root = root;
    vw.is_act = 0;

    return &vw;
}

void app_list_view_delete(void)
{
    memset(&vw, 0, sizeof(vw));
}
