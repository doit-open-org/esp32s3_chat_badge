#include "lvgl.h"
#include "cl_ui.h"
#include "vw_test.h"
#include "esp_log.h"
#include "../../lv_vpg/lv_vpg.h"
static test_view_t vw;

#define TAG "vw_test"

static void _on_btn_cb(lv_event_t *e)
{
    if (!vw.is_act) return;

    cl_button_t *btn = (cl_button_t*)lv_event_get_param(e);
    if (!btn) return;

    switch (btn->id)
    {
        case CL_UI_KEY_MODE:
            if (btn->event == CL_BTN_CLICK)
            {
                // ESP_LOGI(TAG, "mode\n");
                page_change("home");
                // ESP_LOGI(TAG, "kllklkkl\n");
            }    
            break;
        default:
            break;
    }
}

test_view_t* test_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);

    lv_obj_t *img = lv_img_create(root);
    lv_img_set_src(img, "P:/02.jpg");
    lv_obj_center(img);

    lv_obj_t *rect = lv_obj_create(root);                         // 创建矩形对象
    lv_obj_set_size(rect, 120, 80);                               // 宽 120 高 80
    lv_obj_set_style_bg_color(rect, lv_color_make(255, 0, 0), 0); // 填充红色
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);               // 不透明
    lv_obj_set_style_border_color(rect, lv_color_black(), 0);     // 边框黑色
    lv_obj_set_style_border_width(rect, 3, 0);                    // 边框宽 3 px
    lv_obj_set_style_radius(rect, 0, 0);                          // 直角，不要圆角
    lv_obj_center(rect);                                          // 放在屏幕中心
    return &vw;
}

void test_view_delete(void)
{
    /* 如需释放资源在此处理 */
}
