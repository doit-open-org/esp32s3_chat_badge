#include "vm_wifi_config.h"
#include "esp_log.h"
#include "cl_ui.h"
#include "ui_tool.h"
#include "esp_mac.h"

static const char *TAG = "wifi_config_view";
static wifi_config_view_t vw;
LV_FONT_DECLARE(font_puhui_20_4)
LV_FONT_DECLARE(font_puhui_18_4)
LV_IMG_DECLARE(icn_routine)

static const char *get_device_code(void)
{
    static char device_code[32];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_code, sizeof(device_code), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return device_code;
}
// WiFi 连接状态检查定时器回调
static void wifi_check_timer_cb(lv_timer_t *timer)
{
    wifi_config_view_t *view = (wifi_config_view_t *)lv_timer_get_user_data(timer);

    if (!view->is_active)
    {
        ESP_LOGD(TAG, "View not active, skipping check");
        return;
    }

    // 检查 WiFi 是否已连接
    if (ui_tool_is_wifi_connected())
    {
        ESP_LOGI(TAG, "WiFi connected! Jumping to home page...");

        // 停止定时器
        lv_timer_del(view->check_timer);
        view->check_timer = NULL;

        // 跳转到 home 页面
        page_change("home");
    }
    else
    {
        ESP_LOGD(TAG, "WiFi not connected yet, continue waiting...");
    }
}

wifi_config_view_t *wifi_config_view_create(lv_obj_t *root)
{
    ESP_LOGI(TAG, "wifi_config_view_create START, root=%p", root);

    if (root == NULL)
    {
        ESP_LOGE(TAG, "ERROR: root object is NULL!");
        return NULL;
    }

    // 初始化
    vw.check_timer = NULL;
    vw.is_active = false;
    vw.show_device_code = false;
    vw.show_ap_hint = false;

    // 设置背景
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_white(), 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    // 创建图片
    vw.routine_img = lv_img_create(root);
    if (vw.routine_img == NULL)
    {
        ESP_LOGE(TAG, "Failed to create routine_img!");
        return NULL;
    }
    lv_img_set_src(vw.routine_img, &icn_routine);
    lv_obj_align(vw.routine_img, LV_ALIGN_CENTER, 0, -10);

    // 创建配网引导文本（默认显示）
    vw.main_label = lv_label_create(root);
    if (vw.main_label == NULL)
    {
        ESP_LOGE(TAG, "Failed to create main_label!");
        return NULL;
    }
    lv_obj_set_style_text_font(vw.main_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(vw.main_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(vw.main_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(vw.main_label, LV_HOR_RES - 50);
    lv_label_set_long_mode(vw.main_label, LV_LABEL_LONG_WRAP);
    // lv_label_set_text(vw.main_label, "进入四博小助手,创建智能体,\n然后添加设备。");
     lv_label_set_text(vw.main_label, "扫描二维码");
    lv_obj_align(vw.main_label, LV_ALIGN_CENTER, 0, 132);

    // 创建设备码标签（初始隐藏）
    vw.device_code_label = lv_label_create(root);
    if (vw.device_code_label == NULL)
    {
        ESP_LOGE(TAG, "Failed to create device_code_label!");
        return NULL;
    }
    lv_obj_set_style_text_font(vw.device_code_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(vw.device_code_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(vw.device_code_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(vw.device_code_label, LV_HOR_RES - 40);

    char device_code_text[64];
    snprintf(device_code_text, sizeof(device_code_text), "设备码\n%s", get_device_code());
    lv_label_set_text(vw.device_code_label, device_code_text);
    lv_obj_align(vw.device_code_label, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_flag(vw.device_code_label, LV_OBJ_FLAG_HIDDEN);

    // 创建 AP 配网提示标签（初始隐藏）
    vw.ap_hint_label = lv_label_create(root);
    if (vw.ap_hint_label == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ap_hint_label!");
        return NULL;
    }
    lv_obj_set_style_text_font(vw.ap_hint_label, &font_puhui_18_4, 0);
    lv_obj_set_style_text_color(vw.ap_hint_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(vw.ap_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(vw.ap_hint_label, LV_HOR_RES - 40);
    lv_label_set_long_mode(vw.ap_hint_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(vw.ap_hint_label, "");
    lv_obj_align(vw.ap_hint_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(vw.ap_hint_label, LV_OBJ_FLAG_HIDDEN);

    // 创建 AP 配网标题（初始隐藏）
    vw.ap_title_label = lv_label_create(root);
    if (vw.ap_title_label == NULL)
    {
        ESP_LOGE(TAG, "Failed to create ap_title_label!");
        return NULL;
    }
    lv_obj_set_style_text_font(vw.ap_title_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(vw.ap_title_label, lv_color_black(), 0);
    lv_obj_set_style_text_align(vw.ap_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(vw.ap_title_label, "AP配网模式");
    lv_obj_align(vw.ap_title_label, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_add_flag(vw.ap_title_label, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "wifi_config_view_create COMPLETE, device_code=%s", get_device_code());
    return &vw;
}

void wifi_config_view_set_active(bool active)
{
    ESP_LOGI(TAG, "wifi_config_view_set_active: %d", active);
    vw.is_active = active;

    if (active)
    {
        // 页面激活时，创建定时器检查 WiFi 连接状态
        if (vw.check_timer == NULL)
        {
            ESP_LOGI(TAG, "Creating WiFi check timer (check every 1 second)");
            vw.check_timer = lv_timer_create(wifi_check_timer_cb, 1000, &vw);
        }
    }
    else
    {
        // 页面失活时，删除定时器
        if (vw.check_timer != NULL)
        {
            ESP_LOGI(TAG, "Deleting WiFi check timer");
            lv_timer_del(vw.check_timer);
            vw.check_timer = NULL;
        }
    }
}

void wifi_config_view_show_device_code(bool show)
{
    if (vw.main_label == NULL || vw.device_code_label == NULL)
    {
        ESP_LOGE(TAG, "Labels not initialized!");
        return;
    }

    vw.show_device_code = show;

    if (show)
    {
        if (vw.ap_hint_label != NULL)
        {
            lv_obj_add_flag(vw.ap_hint_label, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(vw.main_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(vw.device_code_label, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Showing device code");
    }
    else
    {
        lv_obj_clear_flag(vw.main_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(vw.device_code_label, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Showing config guide");
    }
}

void wifi_config_view_set_ap_hint(const char *hint)
{
    if (vw.main_label == NULL || vw.ap_hint_label == NULL)
    {
        ESP_LOGE(TAG, "Labels not initialized!");
        return;
    }

    vw.show_ap_hint = (hint != NULL && hint[0] != '\0');
    if (vw.show_ap_hint)
    {
        lv_label_set_text(vw.ap_hint_label, hint);
        lv_obj_add_flag(vw.main_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(vw.ap_hint_label, LV_OBJ_FLAG_HIDDEN);
        if (vw.routine_img != NULL)
        {
            lv_obj_add_flag(vw.routine_img, LV_OBJ_FLAG_HIDDEN);
        }
        if (vw.ap_title_label != NULL)
        {
            lv_obj_clear_flag(vw.ap_title_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (vw.device_code_label != NULL)
        {
            lv_obj_add_flag(vw.device_code_label, LV_OBJ_FLAG_HIDDEN);
        }
        ESP_LOGI(TAG, "Showing AP config hint");
    }
    else
    {
        lv_obj_add_flag(vw.ap_hint_label, LV_OBJ_FLAG_HIDDEN);
        if (vw.routine_img != NULL)
        {
            lv_obj_clear_flag(vw.routine_img, LV_OBJ_FLAG_HIDDEN);
        }
        if (vw.ap_title_label != NULL)
        {
            lv_obj_add_flag(vw.ap_title_label, LV_OBJ_FLAG_HIDDEN);
        }
        ESP_LOGI(TAG, "Hiding AP config hint");
    }
}

void wifi_config_view_delete(void)
{
    if (vw.check_timer != NULL)
    {
        lv_timer_del(vw.check_timer);
        vw.check_timer = NULL;
    }

    vw.is_active = false;
    vw.show_device_code = false;
    vw.show_ap_hint = false;
}
