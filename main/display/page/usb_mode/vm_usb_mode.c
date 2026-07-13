#include "vm_usb_mode.h"
#include "cl_ui.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "usb_mode_view";
static usb_mode_view_t vw;

LV_FONT_DECLARE(font_puhui_20_4)
LV_IMG_DECLARE(icon_ukey_80)

static void _on_btn_cb(lv_event_t *e)
{
    if (!vw.is_act)
        return;

    cl_button_t *btn = (cl_button_t *)lv_event_get_param(e);
    if (!btn)
        return;

}

usb_mode_view_t *usb_mode_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    memset(&vw, 0, sizeof(usb_mode_view_t));
    vw.root = root;

    // USB标题
    lv_obj_t *usb_icon = lv_img_create(root);
    lv_img_set_src(usb_icon, &icon_ukey_80);
    lv_obj_align(usb_icon, LV_ALIGN_CENTER, 0, -30);

    // 标题
    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "U盘模式");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    vw.is_act = 0;

    ESP_LOGI(TAG, "USB mode view created");
    return &vw;
}

void usb_mode_view_delete(void)
{
    ESP_LOGI(TAG, "USB mode view deleted");
    memset(&vw, 0, sizeof(usb_mode_view_t));
}
