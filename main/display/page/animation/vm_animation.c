
// #define USE_VPG_DECODE

#include "vm_animation.h"
#include "chat_background.h"
#include "cl_ui.h"
#include "doit_file.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vpg_decode.h"
#include <string.h>

#ifndef USE_VPG_DECODE
// LV_VPG模式：使用LVGL原生控件（默认）
#include "../../lv_vpg/lv_vpg.h"
#else
// VPG_Decode模式：使用doit_ui接口
#include "doit_ui.h"
#endif

#define TAG "vm_animation"

static animation_view_t vw; // 页面数据

LV_FONT_DECLARE(font_puhui_ascii_80_4)
LV_FONT_DECLARE(font_puhui_20_4)

LV_IMG_DECLARE(icon_upload_video_80)
LV_IMG_DECLARE(icon_delete_64)

static void _on_btn_cb(lv_event_t *e)
{
    if (!vw.is_act)
        return;

    cl_button_t *btn = (cl_button_t *)lv_event_get_param(e);
    if (!btn)
        return;

    switch (btn->id)
    {
    case CL_UI_KEY_POWER:
        if (btn->event == CL_BTN_CLICK)
        {
            // 电源键处理
        }
        break;

    case CL_UI_KEY_MODE:
        if (btn->event == CL_BTN_CLICK)
        {
            page_change("home");
        }
        else if (btn->event == CL_BTN_DOUBLE_CLICK && doit_file_has_vpg())
        {
            if (lv_obj_has_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN))
            {
                ESP_LOGI(TAG, "Double click: showing delete dialog");
                lv_obj_clear_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(vw.del_dialog);
            }
            else
            {
                ESP_LOGI(TAG, "Double click: hiding delete dialog");
                lv_obj_add_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
            }
        }
        break;

    default:
        break;
    }
}

static void _on_gesture_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "_on_gesture_cb: is_act=%d, ges_lock=%d", vw.is_act,
             vw.ges_lock);

    if (!vw.is_act)
    {
        ESP_LOGW(TAG, "Gesture ignored: view not active (is_act=%d)", vw.is_act);
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE)
    {
        if (vw.ges_lock)
            return;

        lv_indev_wait_release(lv_indev_active());
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        ESP_LOGI(TAG, "Gesture detected: dir=%d, has_vpg=%d", dir,
                 doit_file_has_vpg());

        switch (dir)
        {
        case LV_DIR_LEFT:
            if (doit_file_has_vpg())
            {
                ESP_LOGI(TAG, "Switching to next vpg...");
                doit_file_switch_next();
            }
            break;

        case LV_DIR_RIGHT:
            if (doit_file_has_vpg())
            {
                ESP_LOGI(TAG, "Switching to previous vpg...");
                doit_file_switch_prev();
            }
            break;

        default:
            break;
        }
    }
}

static void _del_curr_vpg(void)
{
    if (!doit_file_has_vpg())
    {
        ESP_LOGW(TAG, "No vpg to delete");
        return;
    }

    ESP_LOGI(TAG, "Deleting current vpg...");
    doit_file_delete_current();

    if (doit_file_has_vpg())
    {
        if (vw.del_dialog)
        {
            lv_obj_add_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
        }
        animation_view_refresh_vpg();
    }
    else
    {
        ESP_LOGI(TAG, "没有更多VPG，显示上传UI");

        // 隐藏VPG对象
        if (vw.bg_vpg)
        {
            lv_obj_add_flag(vw.bg_vpg, LV_OBJ_FLAG_HIDDEN);
        }

        // 隐藏删除对话框
        if (vw.del_dialog)
        {
            lv_obj_add_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
        }

        // 显示上传提示（保存引用以便后续隐藏）
        lv_obj_t *img = lv_img_create(vw.root);
        lv_img_set_src(img, &icon_upload_video_80);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);
        vw.upload_img = img;

        lv_obj_t *label = lv_label_create(vw.root);
        lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
        lv_label_set_text(label, "请连接小程序上传视频");
        vw.upload_label = label;

        lv_obj_move_foreground(img);
        lv_obj_move_foreground(label);
        ESP_LOGI(TAG, "上传UI已创建: img=%p, label=%p", img, label);
    }
}

static void _on_dialog_cancel(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Dialog cancelled");
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
}

static void _on_dialog_delete(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Dialog delete confirmed, deleting vpg");
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
    _del_curr_vpg();
}

static void _on_dialog_set_background(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Setting current VPG as chat background");

    // 获取当前VPG的SD卡路径
    char *sd_path = get_show_dir();
    if (sd_path == NULL)
    {
        ESP_LOGE(TAG, "Failed to get current VPG path");
        lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    ESP_LOGI(TAG, "Setting background: %s", sd_path);

    // 调用背景管理器设置背景
    if (chat_background_set(sd_path))
    {
        ESP_LOGI(TAG, "Background set successfully");
        // TODO: 可以在这里添加设置成功的提示UI
    }
    else
    {
        ESP_LOGE(TAG, "Failed to set background");
    }

    // 关闭对话框
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *_create_action_dialog(lv_obj_t *parent)
{
    // 创建半透明背景遮罩容器
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_50, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // 创建对话框主体 - 垂直布局，包含3个按钮
    lv_obj_t *dialog = lv_obj_create(container);
    lv_obj_set_size(dialog, 200, 180);
    lv_obj_align(dialog, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(dialog, 20, 0);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 0, 0);
    lv_obj_set_style_pad_all(dialog, 10, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    // "设为聊天背景"按钮
    lv_obj_t *btn_set_bg = lv_btn_create(dialog);
    lv_obj_set_size(btn_set_bg, 180, 45);
    lv_obj_align(btn_set_bg, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(btn_set_bg, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(btn_set_bg, 0, 0);
    lv_obj_set_style_radius(btn_set_bg, 10, 0);
    lv_obj_set_style_shadow_width(btn_set_bg, 0, 0);
    lv_obj_t *btn_set_bg_label = lv_label_create(btn_set_bg);
    lv_obj_set_style_text_font(btn_set_bg_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(btn_set_bg_label, lv_color_hex(0x629fe3), 0);
    lv_label_set_text(btn_set_bg_label, "设为聊天背景");
    lv_obj_center(btn_set_bg_label);

    // "删除"按钮
    lv_obj_t *btn_delete = lv_btn_create(dialog);
    lv_obj_set_size(btn_delete, 180, 45);
    lv_obj_align(btn_delete, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(btn_delete, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(btn_delete, 0, 0);
    lv_obj_set_style_radius(btn_delete, 10, 0);
    lv_obj_set_style_shadow_width(btn_delete, 0, 0);
    lv_obj_t *btn_delete_label = lv_label_create(btn_delete);
    lv_obj_set_style_text_font(btn_delete_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(btn_delete_label, lv_color_hex(0xd2455a), 0);
    lv_label_set_text(btn_delete_label, "删除");
    lv_obj_center(btn_delete_label);

    // "取消"按钮
    lv_obj_t *btn_cancel = lv_btn_create(dialog);
    lv_obj_set_size(btn_cancel, 180, 45);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(btn_cancel, 0, 0);
    lv_obj_set_style_radius(btn_cancel, 10, 0);
    lv_obj_set_style_shadow_width(btn_cancel, 0, 0);
    lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
    lv_obj_set_style_text_font(btn_cancel_label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(btn_cancel_label, lv_color_white(), 0);
    lv_label_set_text(btn_cancel_label, "取消");
    lv_obj_center(btn_cancel_label);

    // 注册按钮事件回调
    lv_obj_add_event_cb(btn_set_bg, _on_dialog_set_background, LV_EVENT_CLICKED,
                        container);
    lv_obj_add_event_cb(btn_delete, _on_dialog_delete, LV_EVENT_CLICKED,
                        container);
    lv_obj_add_event_cb(btn_cancel, _on_dialog_cancel, LV_EVENT_CLICKED,
                        container);

    // 初始隐藏对话框
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);

    return container;
}

void animation_view_refresh_vpg(void)
{
    ESP_LOGI(TAG, "refresh act=%d", vw.is_act);
    if (!vw.is_act)
        return;
    if (!vw.bg_vpg)
        return;
    if (!doit_file_has_vpg())
        return;

    char *sd_path = get_show_dir();
    if (!sd_path)
        return;

#ifdef USE_VPG_DECODE
    // ========== VPG_Decode模式 ==========
    ESP_LOGI(TAG, "VPG_Decode mode: load %s", sd_path);

    /* 【线程安全】添加LVGL锁保护，因为此函数可能从BLE任务调用 */
    // 删除上传提示UI
    ESP_LOGI(TAG, "删除上传UI: img=%p, label=%p", vw.upload_img, vw.upload_label);
    lvgl_port_lock(-1);
    if (vw.upload_img)
    {
        lv_obj_del(vw.upload_img);
        vw.upload_img = NULL;
    }
    if (vw.upload_label)
    {
        lv_obj_del(vw.upload_label);
        vw.upload_label = NULL;
    }
    // 确保显示对象可见
    lv_obj_clear_flag(vw.bg_vpg, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
    ESP_LOGI(TAG, "上传UI已删除，开始显示VPG");

    // 先停止当前播放，再启动新文件
    doit_vpg_player_stop();
    doit_file_decode();

#else
    // ========== LV_VPG模式（默认） ==========
    static char lvgl_path[128];
    if (strncmp(sd_path, "/sdcard/", 8) == 0)
        snprintf(lvgl_path, sizeof(lvgl_path), "P:%s", sd_path + 7);
    else
    {
        strncpy(lvgl_path, sd_path, sizeof(lvgl_path) - 1);
        lvgl_path[sizeof(lvgl_path) - 1] = '\0';
    }

    ESP_LOGI(TAG, "LV_VPG mode: load %s", lvgl_path);
    // 删除上传提示UI
    ESP_LOGI(TAG, "删除上传UI: img=%p, label=%p", vw.upload_img, vw.upload_label);

    /* 【线程安全】添加LVGL锁保护，因为此函数可能从BLE任务调用 */
    lvgl_port_lock(-1);
    if (vw.upload_img)
    {
        lv_obj_del(vw.upload_img);
        vw.upload_img = NULL;
    }
    if (vw.upload_label)
    {
        lv_obj_del(vw.upload_label);
        vw.upload_label = NULL;
    }
    lv_obj_add_flag(vw.bg_vpg, LV_OBJ_FLAG_HIDDEN); // 隐藏VPG对象
    // 设置VPG源并确保可见
    lv_vpg_set_src(vw.bg_vpg, lvgl_path);
    lvgl_port_unlock();
    ESP_LOGI(TAG, "上传UI已删除，开始显示VPG");

    vTaskDelay(pdMS_TO_TICKS(30));

    lvgl_port_lock(-1);
    lv_obj_clear_flag(vw.bg_vpg, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
#endif
}

animation_view_t *animation_view_create(lv_obj_t *root, const char *path)
{
    // 设置为VPG动画模式
    doit_file_set_type(MATERIAL_TYPE_VPG);
    ESP_LOGI(TAG, "Set file type to VPG, has_vpg=%d", doit_file_has_vpg());

    // 注册刷新显示回调
    doit_file_set_refresh_display_callback(animation_view_refresh_vpg);

    // 设置根对象样式
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

#ifdef USE_VPG_DECODE
    // ========== VPG_Decode模式：使用lv_image + doit_vpg_player ==========
    ESP_LOGI(TAG, "Using VPG_Decode mode (doit_vpg_player)");

    // 创建标准lv_image对象用于显示
    lv_obj_t *vpg_obj = lv_image_create(root);
    lv_obj_set_size(vpg_obj, 368, 368);
    lv_obj_align(vpg_obj, LV_ALIGN_CENTER, 0, 0);

    // 初始化doit_ui，注册显示对象
    doit_file_page_init(vpg_obj, 368, 368);
#else
    // ========== LV_VPG模式：使用LVGL原生控件（默认） ==========
    ESP_LOGI(TAG, "Using LV_VPG mode (lv_vpg widget)");

    // 创建VPG显示对象
    lv_obj_t *vpg_obj = lv_vpg_create(root);
    lv_obj_set_size(vpg_obj, 368, 368);
    lv_obj_align(vpg_obj, LV_ALIGN_CENTER, 0, 0);

    // 初始化doit_ui
    doit_file_page_init(vpg_obj, 368, 368);
#endif

    // 初始化上传UI指针
    vw.upload_img = NULL;
    vw.upload_label = NULL;

    if (doit_file_has_vpg())
    {
#ifdef USE_VPG_DECODE
        // VPG_Decode模式：调用doit_file_decode()
        doit_file_decode();
#else
        // LV_VPG模式：使用lv_vpg_set_src()
        char *sd_path = get_show_dir();
        if (sd_path == NULL)
        {
            ESP_LOGE(TAG, "get_show_dir() returned NULL");
        }
        else
        {
            static char lvgl_path[128];
            if (strncmp(sd_path, "/sdcard/", 8) == 0)
            {
                snprintf(lvgl_path, sizeof(lvgl_path), "P:%s", sd_path + 7);
            }
            else
            {
                strncpy(lvgl_path, sd_path, sizeof(lvgl_path) - 1);
            }

            ESP_LOGI(TAG, "Loading vpg: %s", lvgl_path);
            lv_vpg_set_src(vpg_obj, lvgl_path);
        }
#endif
    }
    else
    {
        ESP_LOGI(TAG, "没有VPG文件，显示上传UI");
        lv_obj_add_flag(vpg_obj, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *img = lv_img_create(root);
        lv_img_set_src(img, &icon_upload_video_80);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);
        vw.upload_img = img;

        lv_obj_t *label = lv_label_create(root);
        lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
        lv_label_set_text(label, "请连接小程序上传视频");
        vw.upload_label = label;

        lv_obj_move_background(label);
        lv_obj_move_background(img);
        ESP_LOGI(TAG, "上传UI已创建: img=%p, label=%p", img, label);
    }

    // 创建操作对话框（设为背景/删除/取消）
    vw.del_dialog = _create_action_dialog(root);

    // 注册事件回调
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    lv_obj_add_event_cb(root, _on_gesture_cb, LV_EVENT_GESTURE, NULL);

    // 保存对象引用
    vw.bg_vpg = vpg_obj;
    vw.root = root;
    vw.vpg_handle = (void *)1;

    // 初始化状态
    vw.is_act = 0;
    vw.ges_lock = 0;
    vw.delete_cont = NULL;
    vw.delete_bar = NULL;
    return &vw;
}

void animation_view_delete(void) { /* 如需释放资源在此处理 */ }
