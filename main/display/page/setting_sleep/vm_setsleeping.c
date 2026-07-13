#include "vm_setsleeping.h"
#include "lvgl.h"
#include "ui_tool.h"
#include "page_manager.h"
#include "cl_ui.h"

LV_FONT_DECLARE(font_puhui_20_4)
LV_IMG_DECLARE(bottom_btn)

static setting_sleep_view_t vw = {0};
#define _this vw

// 按键事件处理
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

static void _on_back_btn(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        page_change("setting");
    }
}

static void _on_item_click(lv_event_t *e)
{
    int sleep_time = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (_this.set_curr == obj)
    {
        return;
    }

    if (_this.set_curr)
    {
        lv_obj_set_style_bg_color(_this.set_curr, lv_color_hex(0x1c1c1c), 0);
    }
    _this.set_curr = obj;
    // xiaozhi_board_set_sleep_time((u32)sleep_time);
    ui_tool_set_sleep_time((uint32_t)sleep_time);
    lv_obj_set_style_bg_color(_this.set_curr, lv_color_hex(0x53a9ff), 0);
}

static lv_obj_t *creat_sleep_time_item(lv_obj_t *parent, const char *name, int sleep_sec)
{

    lv_obj_t *priv = NULL;
    int count = lv_obj_get_child_count(parent);
    if (count != 0)
    {
        priv = lv_obj_get_child(parent, count - 1);
    }

    // 创建容器，布局水平排列，宽度和父容器一样
    lv_obj_t *cont = lv_button_create(parent);
    lv_obj_set_size(cont, 300, 55);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, 200, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_radius(cont, 25, 0);
    lv_obj_set_style_shadow_width(cont, 0, 0);
    lv_obj_set_style_shadow_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_ofs_x(cont, 0, 0);
    lv_obj_set_style_shadow_ofs_y(cont, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, name);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 20, 0);

    if (ui_tool_get_sleep_time() == sleep_sec)
    {
        vw.set_curr = cont;
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x53A9FF), 0);
    }

    lv_obj_add_event_cb(cont, _on_item_click, LV_EVENT_CLICKED, (void *)sleep_sec);

    return cont;
}

setting_sleep_view_t *setting_sleep_view_create(lv_obj_t *root)
{

    vw.set_curr = NULL;
    vw.root = root;
    vw.set_target = NULL;

    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(root);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    lv_obj_t *title_label = lv_label_create(root);
    lv_obj_set_style_text_font(title_label, &font_puhui_20_4, 0);
    lv_label_set_text(title_label, "自动关机");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xe9feff), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *item_cont = lv_obj_create(root);
    lv_obj_align(item_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_size(item_cont, LV_HOR_RES, LV_VER_RES - 80);
    lv_obj_set_style_border_width(item_cont, 0, 0);
    lv_obj_set_style_bg_color(item_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(item_cont, 0, 0);
    lv_obj_set_scrollbar_mode(item_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(item_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(item_cont, 5, 0);
    lv_obj_set_flex_align(item_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    creat_sleep_time_item(item_cont, "从不", 0);
    creat_sleep_time_item(item_cont, "5分钟", 5 * 60);
    creat_sleep_time_item(item_cont, "10分钟", 10 * 60);
    creat_sleep_time_item(item_cont, "30分钟", 30 * 60);
    creat_sleep_time_item(item_cont, "1小时", 60 * 60);
    creat_sleep_time_item(item_cont, "2小时", 2 * 60 * 60);
    creat_sleep_time_item(item_cont, "3小时", 3 * 60 * 60);
    creat_sleep_time_item(item_cont, "4小时", 4 * 60 * 60);
    creat_sleep_time_item(item_cont, "5小时", 5 * 60 * 60);
    creat_sleep_time_item(item_cont, "6小时", 6 * 60 * 60);

    lv_obj_t *btn_back = lv_img_create(root);
    lv_img_set_src(btn_back, &bottom_btn);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label_back;
    label_back = lv_label_create(btn_back);
    lv_obj_set_width(label_back, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(label_back, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(label_back, LV_ALIGN_CENTER);
    lv_label_set_text(label_back, "返回");
    lv_obj_set_style_text_color(label_back, lv_color_hex(0x629fe3), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_back, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn_back, _on_back_btn, LV_EVENT_CLICKED, NULL);

    if (vw.set_curr)
    {
        lv_group_focus_obj(vw.set_curr);
    }

    vw.is_act = 0;

    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);

    return &vw;
}

void setting_sleep_view_delete(void) {}