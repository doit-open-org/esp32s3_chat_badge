#include "vw_home.h"
#include "../../lv_vpg/lv_vpg.h"
#include "chat_background.h"
#include "cl_ui.h"
#include "doit_file.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include <string.h>
#include <strings.h>

static home_view_t vw;

LV_FONT_DECLARE(font_puhui_ascii_80_4)
LV_FONT_DECLARE(font_puhui_20_4)

LV_IMG_DECLARE(icon_upload_80)
LV_IMG_DECLARE(icon_delete_64)
LV_IMG_DECLARE(bat_empty)
LV_IMG_DECLARE(icon_back)

#define TAG "vw_home"

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
        // 电源键处理
        if (btn->event == CL_BTN_CLICK)
        {
#ifndef SIMULATOR
// extern void toggleChatState();
// toggleChatState();
#endif
        }
        break;

    case CL_UI_KEY_MODE:
        if (btn->event == CL_BTN_CLICK)
        {
            page_change("app_list");
        }
        else if (btn->event == CL_BTN_DOUBLE_CLICK && doit_file_has_images())
        {
            // 双击切换对话框显示/隐藏状态
            if (lv_obj_has_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN))
            {
                ESP_LOGI(TAG, "Double click: showing delete dialog");
                lv_obj_clear_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(vw.del_dialog); // 确保对话框在最上层
            }
            else
            {
                ESP_LOGI(TAG, "Double click: hiding delete dialog");
                lv_obj_add_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
            }
        }
        else if (btn->event == CL_BTN_DOUBLE_CLICK)
        {
            ESP_LOGW(TAG, "Double click ignored: no images available");
        }
        break;

    default:
        break;
    }
}

static void _on_gesture_cb(lv_event_t *e)
{
    if (!vw.is_act)
    {
        ESP_LOGW(TAG, ">>> Gesture REJECTED: view not active (is_act=%d)",
                 vw.is_act);
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE)
    {
        // 如果手势被锁定，不处理
        if (vw.ges_lock)
        {
            ESP_LOGW(TAG, ">>> Gesture REJECTED: gesture locked (ges_lock=%d)",
                     vw.ges_lock);
            return;
        }

        lv_indev_wait_release(lv_indev_active());
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

        switch (dir)
        {
        case LV_DIR_LEFT:
            // 向左滑动：切换到下一张图片
            if (doit_file_has_images())
            {
                doit_file_switch_next();
            }
            else
            {
                ESP_LOGW(TAG, ">>> Cannot switch: no images available");
            }
            break;

        case LV_DIR_RIGHT:
            // 向右滑动：切换到上一张图片
            if (doit_file_has_images())
            {
                doit_file_switch_prev();
            }
            else
            {
                ESP_LOGW(TAG, ">>> Cannot switch: no images available");
            }
            break;

        default:
            ESP_LOGW(TAG, ">>> Unknown gesture direction: %d", dir);
            break;
        }
    }
}

static void _del_curr_jpg()
{
    if (!doit_file_has_images())
    {
        ESP_LOGW(TAG, "No images to delete, already showing last state");
        return;
    }

    ESP_LOGI(TAG, "Deleting current image...");
    doit_file_delete_current();

    if (doit_file_has_images())
    {
        // doit_file_decode();
        if (vw.del_dialog)
        {
            lv_obj_add_flag(vw.del_dialog, LV_OBJ_FLAG_HIDDEN);
        }
        home_view_refresh_image();
    }
    else
    {
        // 清空背景图片
        if (vw.bg_img)
        {
            lv_image_cache_drop(lv_img_get_src(vw.bg_img));
            lv_img_set_src(vw.bg_img, NULL);
        }

        lv_obj_t *img = lv_img_create(vw.root);
        lv_img_set_src(img, &icon_upload_80);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);

        lv_obj_t *label = lv_label_create(vw.root);
        lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
        lv_label_set_text(label, "请连接小程序上传图片");

        // 移到前景，确保可见
        // lv_obj_move_foreground(img);
        lv_obj_move_foreground(label);
    }
}

static lv_obj_t *_create_delete_cont(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 260, 260);
    lv_obj_set_style_radius(cont, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_center(cont);
    lv_obj_set_style_shadow_width(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, &icon_delete_64);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_label_set_text(label, "长按删除图片");

    lv_obj_t *bar = lv_arc_create(cont);
    lv_obj_set_style_arc_width(bar, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(bar, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_remove_style(bar, NULL,
                        LV_PART_KNOB); /*Be sure the knob is not displayed*/
    lv_obj_remove_flag(bar,
                       LV_OBJ_FLAG_CLICKABLE); /*To not allow adjusting by click*/
    lv_obj_set_size(bar, 260, 260);
    lv_obj_center(bar);
    lv_arc_set_bg_angles(bar, 0, 360);
    lv_arc_set_rotation(bar, -90);
    lv_arc_set_value(bar, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);

    vw.delete_cont = cont;
    vw.delete_bar = bar;

    return cont;
}

static void _on_dialog_cancel(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Dialog cancelled, hiding dialog");
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
}

static void _on_dialog_delete(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Dialog delete confirmed, hiding dialog and deleting image");
    lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
    _del_curr_jpg();
    ESP_LOGI(TAG, "After delete, dialog hidden flag: %d",
             lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
}

static void _on_dialog_set_background(lv_event_t *e)
{
    lv_obj_t *dialog = lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Setting current image as chat background");

    // 获取当前图片的SD卡路径
    char *sd_path = get_show_dir();
    if (sd_path == NULL)
    {
        ESP_LOGE(TAG, "Failed to get current image path");
        lv_obj_add_flag(dialog, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    ESP_LOGI(TAG, "Setting background: %s", sd_path);

    // 调用背景管理器设置背景
    if (chat_background_set(sd_path))
    {
        ESP_LOGI(TAG, "Background set successfully");
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
home_view_t *home_view_create(lv_obj_t *root, const char *path)
{
    doit_file_set_type(MATERIAL_TYPE_IMG);
    ESP_LOGI(TAG, "Set file type to IMG, has_images=%d", doit_file_has_images());

    // 注册刷新显示回调
    doit_file_set_refresh_display_callback(home_view_refresh_image);

    // 【修复】清理可能存在的旧上传UI对象（页面重新创建时）
    if (vw.upload_img)
    {
        ESP_LOGI(TAG, "Cleaning up old upload UI: img=%p, label=%p", vw.upload_img,
                 vw.upload_label);
        lv_obj_del(vw.upload_img);
        vw.upload_img = NULL;
    }
    if (vw.upload_label)
    {
        lv_obj_del(vw.upload_label);
        vw.upload_label = NULL;
    }

    // 设置根对象样式
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *img_bg = lv_img_create(root);
    lv_obj_set_size(img_bg, 360, 360);
    lv_obj_align(img_bg, LV_ALIGN_CENTER, 0, 0);

    doit_file_page_init(img_bg, 368, 368);

    // 初始化上传UI指针
    vw.upload_img = NULL;
    vw.upload_label = NULL;

    if (doit_file_has_images())
    {
        char *sd_path = get_show_dir();
        if (sd_path == NULL)
        {
            ESP_LOGE(TAG, "get_show_dir() returned NULL");
        }
        else
        {
            // 转换为LVGL路径格式: /sdcard/photo/01.jpg -> P:/photo/01.jpg
            static char lvgl_path[128];
            if (strncmp(sd_path, "/sdcard/", 8) == 0)
            {
                snprintf(lvgl_path, sizeof(lvgl_path), "P:%s", sd_path + 7);
            }
            else
            {
                strncpy(lvgl_path, sd_path, sizeof(lvgl_path) - 1);
            }

            ESP_LOGI(TAG, "Loading image: %s", lvgl_path);
            lv_img_set_src(img_bg, lvgl_path);
        }
    }
    else
    {
        ESP_LOGI(TAG, "No images, showing upload UI");
        lv_obj_t *img = lv_img_create(root);
        lv_img_set_src(img, &icon_upload_80);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, -20);
        vw.upload_img = img;

        lv_obj_t *label = lv_label_create(root);
        lv_obj_set_style_text_font(label, &font_puhui_20_4, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 60);
        lv_label_set_text(label, "请连接APP上传图片");
        vw.upload_label = label;

        lv_obj_move_background(label);
        lv_obj_move_background(img);
        ESP_LOGI(TAG, "上传UI已创建: img=%p, label=%p", img, label);
    }

    vw.del_dialog = _create_action_dialog(root);

    lv_obj_align(img_bg, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    lv_obj_add_event_cb(root, _on_gesture_cb, LV_EVENT_GESTURE, NULL);

    vw.bg_img = img_bg;
    vw.root = root;
    vw.jpg_handle = (void *)1;

    // 初始化状态
    vw.is_act = 0;   // 初始为未激活，等待 on_view_did_appear 设置
    vw.ges_lock = 0; // 初始手势未锁定
    return &vw;
}

void home_view_delete(void) { /* 如需释放资源在此处理 */ }

void home_view_refresh_image(void)
{
    if (!vw.is_act)
    {
        ESP_LOGW(TAG, "Refresh skipped: page not active");
        return;
    }
    if (!vw.bg_img)
    {
        ESP_LOGW(TAG, "Refresh skipped: bg_img is NULL");
        return;
    }
    if (!doit_file_has_images())
    {
        ESP_LOGW(TAG, "Refresh skipped: no images");
        return;
    }

    char *sd_path = get_show_dir();
    if (!sd_path)
    {
        ESP_LOGW(TAG, "Refresh skipped: get_show_dir returned NULL");
        return;
    }

    static char lvgl_path[128];
    if (strncmp(sd_path, "/sdcard/", 8) == 0)
        snprintf(lvgl_path, sizeof(lvgl_path), "P:%s", sd_path + 7);
    else
        strncpy(lvgl_path, sd_path, sizeof(lvgl_path) - 1);

    ESP_LOGI(TAG, "删除上传UI: img=%p, label=%p", vw.upload_img, vw.upload_label);
    lvgl_port_lock(-1);

    // 【修复】如果引用为NULL，尝试通过遍历root的子对象来查找并删除上传UI
    if (!vw.upload_img && !vw.upload_label && vw.root)
    {
        ESP_LOGI(TAG,
                 "Upload UI references are NULL, searching in root children...");
        uint32_t child_cnt = lv_obj_get_child_count(vw.root);
        ESP_LOGI(TAG, "Root has %lu children", (unsigned long)child_cnt);

        // 从后往前遍历，避免删除时索引变化
        for (int32_t i = child_cnt - 1; i >= 0; i--)
        {
            lv_obj_t *child = lv_obj_get_child(vw.root, i);
            if (child && child != vw.bg_img && child != vw.del_dialog)
            {
                // 检查是否是图片或标签对象
                if (lv_obj_check_type(child, &lv_image_class) ||
                    lv_obj_check_type(child, &lv_label_class))
                {
                    ESP_LOGI(TAG, "Found and deleting potential upload UI child: %p",
                             child);
                    lv_obj_del(child);
                }
            }
        }
    }

    // 删除上传提示UI（如果引用存在）
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

    // 刷新图片
    lv_image_cache_drop(lv_img_get_src(vw.bg_img));
    lv_img_set_src(vw.bg_img, lvgl_path);
    lvgl_port_unlock();
    ESP_LOGI(TAG, "上传UI已删除，图片已刷新: %s", lvgl_path);
}
