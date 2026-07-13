#include "cl_ui.h"
#include "esp_log.h"
#include <string.h>
static const char* TAG = "help_view";
#include "vm_help.h"
static help_view_t vw;

LV_FONT_DECLARE(font_puhui_20_4)
LV_IMG_DECLARE(bottom_btn)

static void _on_btn_next(lv_event_t *e)
{
    if (!vw.is_act) {
        return;
    }
    for (uint8_t i = 0; i < sizeof(vw.cont_list)/sizeof(vw.cont_list[0]); i++)
    {
        if (lv_obj_has_flag(vw.cont_list[i], LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(vw.cont_list[i], LV_OBJ_FLAG_HIDDEN);
            return;
        }
    }
    page_change("setting");
}
lv_obj_t *_create_basic_help(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF); 
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label, "设备配网");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *label_desc = lv_label_create(cont);
    lv_obj_set_width(label_desc, 280);
    lv_label_set_long_mode(label_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label_desc, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label_desc, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_desc, "使用四博小程序进行配网");
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 40, -20);

    lv_obj_t *next_btn = lv_button_create(cont);
    lv_obj_set_size(next_btn, 120, 50);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_set_style_border_width(next_btn, 0, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x000000), 0);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_t *label_next = lv_label_create(next_btn);
    lv_obj_set_style_text_font(label_next, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label_next, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_next, "下一页 >");
    lv_obj_center(label_next);
    lv_obj_add_event_cb(next_btn, _on_btn_next, LV_EVENT_CLICKED, NULL);

    return cont;
}

// 创建功能介绍帮助页面
lv_obj_t *_create_function_help(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF); 
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE); 
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label, "功能介绍");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *label_desc = lv_label_create(cont);
    lv_obj_set_width(label_desc, 280);
    lv_label_set_long_mode(label_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label_desc, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label_desc, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_desc, "• 闹钟：设置提醒时间\n• 设置：系统参数配置\n• 亮度：自动调节屏幕\n• 音量：调节系统音量");
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 30, -20);

    lv_obj_t *next_btn = lv_button_create(cont);
    lv_obj_set_size(next_btn, 120, 50);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_set_style_border_width(next_btn, 0, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x000000), 0);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_t *label_next = lv_label_create(next_btn);
    lv_obj_set_style_text_font(label_next, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label_next, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_next, "下一页 >");
    lv_obj_center(label_next);
    lv_obj_add_event_cb(next_btn, _on_btn_next, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    return cont;
}

// 创建联系支持帮助页面
lv_obj_t *_create_support_help(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF); 
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label, "技术支持");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *label_desc = lv_label_create(cont);
    lv_obj_set_width(label_desc, 280);
    lv_label_set_long_mode(label_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label_desc, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label_desc, lv_color_hex(0xffffff), 0);
    lv_label_set_text(label_desc, "如有问题请联系技术支持\n\n• 重启设备可解决大部分问题\n• 恢复出厂设置在设置页面\n• 更多帮助请查看用户手册");
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 0, -20);

    // 返回按钮
    lv_obj_t *btn_back = lv_image_create(cont);
    lv_image_set_src(btn_back, &bottom_btn);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label_back;
    label_back = lv_label_create(btn_back);
    lv_obj_set_width(label_back, LV_SIZE_CONTENT);
    lv_obj_set_height(label_back, LV_SIZE_CONTENT);
    lv_obj_set_align(label_back, LV_ALIGN_CENTER);
    lv_label_set_text(label_back, "返回");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0x629fe3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_back, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_back, _on_btn_next, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    return cont;
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
        if (btn->event == CL_BTN_CLICK)
        {
            page_change("home");
        }
        break;

    default:
        break;
    }
}

help_view_t* help_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    memset(&vw, 0, sizeof(help_view_t));
    vw.cont_list[0] = _create_basic_help(root);
    vw.cont_list[1] = _create_function_help(root);
    vw.cont_list[2] = _create_support_help(root);
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    vw.is_act = 0;
    return &vw;
}

void help_view_delete(void)
{
    memset(&vw, 0, sizeof(help_view_t));
}
