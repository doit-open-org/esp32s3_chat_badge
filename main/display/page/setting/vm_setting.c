#include "vm_setting.h"
#include "cl_ui.h"
#include "assert/language.h"
#include "wifi_board_c_api.h"

static setting_view_t vw;
LV_FONT_DECLARE(font_puhui_20_4)

LV_IMG_DECLARE(icon_setting_32)
LV_IMG_DECLARE(icon_power_32)
LV_IMG_DECLARE(icon_voice_32)
LV_IMG_DECLARE(icon_help_32)
LV_IMG_DECLARE(icon_about_32)
LV_IMG_DECLARE(icon_sys_rst)
LV_IMG_DECLARE(bottom_btn)

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

static lv_obj_t *creat_setting_item(lv_obj_t *parent, const void *icon_src, const char *title, void (*on_click)(lv_event_t *e))
{
    lv_obj_t *cont = lv_button_create(parent);
    lv_obj_set_size(cont, 300, 55);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, 200, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1c1c1c), 0);
    lv_obj_set_style_radius(cont, 25, 0);
    lv_obj_set_style_shadow_width(cont, 0, 0);

    lv_obj_t *icon = lv_image_create(cont);
    lv_image_set_src(icon, icon_src);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, title);
    lv_obj_align_to(label, icon, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    if (on_click != NULL)
    {
        lv_obj_add_event_cb(cont, on_click, LV_EVENT_CLICKED, cont);
    }
    return cont;
}
static void _on_help_click(lv_event_t *e)
{
    page_change("help");
}

static void _on_auto_sleep_click(lv_event_t *e)
{
    page_change("setting_sleep");
}

static void _on_about_click(lv_event_t *e)
{
    page_change("about");
}

static void _on_role_click(lv_event_t *e)
{
    
}

static void _on_back_btn(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        page_change("home");
    }
}

static void _on_dialog_cancel(lv_event_t *e)
{
    lv_obj_add_flag(vw.rst_dialog, LV_OBJ_FLAG_HIDDEN);
}

static void _on_dialog_confirm(lv_event_t *e)
{
    lv_obj_add_flag(vw.rst_dialog, LV_OBJ_FLAG_HIDDEN);

    wifi_board_reset_configuration();
}

static lv_obj_t *_create_sys_rst_dialog(lv_obj_t *parent)
{
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_50, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dialog = lv_obj_create(container);
    lv_obj_set_size(dialog, 240, 120);
    lv_obj_align(dialog, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(dialog, 20, 0);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 0, 0);
    lv_obj_set_style_pad_all(dialog, 5, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(dialog);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "恢复出厂设置?");

    lv_obj_t *btn_cancel = lv_button_create(dialog);
    lv_obj_set_size(btn_cancel, 120, 40);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, -5, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_cancel, 0, 0);
    lv_obj_set_style_radius(btn_cancel, 20, 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
    lv_obj_set_style_text_font(btn_cancel_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(btn_cancel_label, lv_color_hex(0x629fe3), 0);
    lv_label_set_text(btn_cancel_label, "取消");
    lv_obj_center(btn_cancel_label);

    lv_obj_t *btn_confirm = lv_button_create(dialog);
    lv_obj_set_size(btn_confirm, 120, 40);
    lv_obj_align(btn_confirm, LV_ALIGN_BOTTOM_RIGHT, 5, 0);
    lv_obj_set_style_bg_color(btn_confirm, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(btn_confirm, 0, 0);
    lv_obj_set_style_radius(btn_confirm, 20, 0);
    lv_obj_set_style_shadow_width(btn_confirm, 0, 0);
    lv_obj_t *btn_confirm_label = lv_label_create(btn_confirm);
    lv_obj_set_style_text_font(btn_confirm_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(btn_confirm_label, lv_color_hex(0xd2455a), 0);
    lv_label_set_text(btn_confirm_label, "确定");
    lv_obj_center(btn_confirm_label);

    lv_obj_t *footer = lv_obj_create(dialog);
    lv_obj_set_size(footer, 2, 20);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x4A505A), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_add_event_cb(btn_cancel, _on_dialog_cancel, LV_EVENT_CLICKED, container);
    lv_obj_add_event_cb(btn_confirm, _on_dialog_confirm, LV_EVENT_CLICKED, container);

    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);

    return container;
}

static void _on_system_rst_menu(lv_event_t *e)
{
    if (lv_obj_has_flag(vw.rst_dialog, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_clear_flag(vw.rst_dialog, LV_OBJ_FLAG_HIDDEN);
    }
}

setting_view_t *setting_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    // 创建标题
    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "设置");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *cont = lv_obj_create(root);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 120);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 20, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont, 5, 0);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 创建设置项目
    creat_setting_item(cont, &icon_help_32, "帮助", _on_help_click);
    creat_setting_item(cont, &icon_sys_rst, "还原", _on_system_rst_menu);
    creat_setting_item(cont, &icon_about_32, "关于", _on_about_click);
    creat_setting_item(cont, &icon_power_32, "自动关机", _on_auto_sleep_click);

    // 创建隐藏的信息容器
    vw.cont_info = lv_obj_create(root);
    lv_obj_add_flag(vw.cont_info, LV_OBJ_FLAG_HIDDEN);

    // 创建隐藏的重置对话框
    vw.rst_dialog = _create_sys_rst_dialog(root);

    // 返回按钮
    lv_obj_t *btn_back = lv_image_create(root);
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
    lv_obj_add_event_cb(btn_back, _on_back_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    vw.is_act = 0;
    return &vw;
}

void setting_view_delete(void)
{
}
