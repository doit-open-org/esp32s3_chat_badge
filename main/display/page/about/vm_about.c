#include "vm_about.h"
#include "cl_ui.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include <string.h>

static const char* TAG = "about_view";
static about_view_t vw;

LV_FONT_DECLARE(font_puhui_20_4)
LV_IMG_DECLARE(bottom_btn)

static const char* get_build_date_iso(void) {
    return "2025-11-23";
}

// 获取设备ID 
static const char* get_device_id(void) {
    static char device_id[32];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return device_id;
}

// 创建信息项
static lv_obj_t* create_info_item(lv_obj_t *parent, const char* label_text, const char* value_text)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 260, 35);
    lv_obj_set_style_pad_all(cont, 5, 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0x535353), 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_radius(cont, 5, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // 检查值文本长度，决定布局方式
    if (lv_text_get_width(value_text, strlen(value_text), &font_puhui_20_4, 0) >= 140) {
        // 长文本：垂直布局
        lv_obj_set_size(cont, 260, 60);
        lv_obj_set_style_pad_bottom(cont, 2, 0);
    } else {
        // 短文本：水平布局
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }

    // 标签
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);

    // 值
    lv_obj_t *value = lv_label_create(cont);
    lv_label_set_text(value, value_text);
    lv_obj_set_style_text_color(value, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(value, &font_puhui_20_4, 0);

    // 长文本特殊处理
    if (lv_text_get_width(value_text, strlen(value_text), &font_puhui_20_4, 0) >= 140) {
        lv_obj_set_width(value, 250);
        lv_label_set_long_mode(value, LV_LABEL_LONG_SCROLL);
        lv_obj_align_to(value, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    }

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

// 返回按钮点击事件
static void _on_back_btn(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        page_change("setting");
    }
}

about_view_t* about_view_create(lv_obj_t *root)
{ 
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    memset(&vw, 0, sizeof(about_view_t));

    // 标题
    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "关于");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // 主内容区
    lv_obj_t *main_cont = lv_obj_create(root);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES-80);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_black(), 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_pad_left(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 信息容器
    vw.info_cont = lv_obj_create(main_cont);
    lv_obj_set_size(vw.info_cont, 280, 220);
    lv_obj_set_style_pad_all(vw.info_cont, 10, 0);
    lv_obj_set_style_border_width(vw.info_cont, 0, 0);
    lv_obj_set_style_bg_color(vw.info_cont, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_radius(vw.info_cont, 10, 0);
    lv_obj_set_scrollbar_mode(vw.info_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(vw.info_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(vw.info_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vw.info_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(vw.info_cont, LV_ALIGN_CENTER, 0, -20);

    // 创建系统信息项
    const esp_app_desc_t *app_desc = esp_app_get_description();
    
    create_info_item(vw.info_cont, "设备名称", "AI BOX");
    create_info_item(vw.info_cont, "软件版本", app_desc->version);
    create_info_item(vw.info_cont, "构建日期", get_build_date_iso());
    create_info_item(vw.info_cont, "设备ID", get_device_id());
    create_info_item(vw.info_cont, "ESP-IDF", app_desc->idf_ver);

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

void about_view_delete(void)
{
    memset(&vw, 0, sizeof(about_view_t));
}
