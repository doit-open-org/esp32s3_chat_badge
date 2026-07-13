#include "vw_ai.h"
#include "cl_ui.h"
#include "lvgl.h"
#include "device_state.h"
#include "ui_tool.h"
#include "chat_background.h"
#include "../../lv_vpg/lv_vpg.h"
#include "string.h"
#include "stdio.h"
#include <strings.h>
#include <esp_log.h>
#define TAG "ai_vw"

// ============================================================
// 表情显示模式配置
// 取消下面这行的注释以启用眼睛动画模式
// 默认使用静态Emoji图片显示模式（复用xiaozhi-fonts资源）
// ============================================================
// #define CONFIG_USE_EYE_ANIMATION

#ifdef CONFIG_USE_EYE_ANIMATION
// 眼睛动画模式：使用多帧图片实现动态表情效果
extern void eye_set_emotion(int emo);
extern lv_obj_t *create_emotion_eye(lv_obj_t *parent);
#else
// Emoji显示模式：使用xiaozhi-fonts中的静态emoji图片（64x64像素）
extern const lv_image_dsc_t emoji_1f636_64; // neutral
extern const lv_image_dsc_t emoji_1f642_64; // happy
extern const lv_image_dsc_t emoji_1f606_64; // laughing
extern const lv_image_dsc_t emoji_1f602_64; // funny
extern const lv_image_dsc_t emoji_1f614_64; // sad
extern const lv_image_dsc_t emoji_1f620_64; // angry
extern const lv_image_dsc_t emoji_1f62d_64; // crying
extern const lv_image_dsc_t emoji_1f60d_64; // loving
extern const lv_image_dsc_t emoji_1f633_64; // embarrassed
extern const lv_image_dsc_t emoji_1f62f_64; // surprised
extern const lv_image_dsc_t emoji_1f631_64; // shocked
extern const lv_image_dsc_t emoji_1f914_64; // thinking
extern const lv_image_dsc_t emoji_1f609_64; // winking
extern const lv_image_dsc_t emoji_1f60e_64; // cool
extern const lv_image_dsc_t emoji_1f60c_64; // relaxed
extern const lv_image_dsc_t emoji_1f924_64; // delicious
extern const lv_image_dsc_t emoji_1f618_64; // kissy
extern const lv_image_dsc_t emoji_1f60f_64; // confident
extern const lv_image_dsc_t emoji_1f634_64; // sleepy
extern const lv_image_dsc_t emoji_1f61c_64; // silly
extern const lv_image_dsc_t emoji_1f644_64; // confused

// Emoji名称到图片的映射表
typedef struct
{
    const char *name;
    const lv_image_dsc_t *image;
} emoji_image_map_t;

static const emoji_image_map_t s_emoji_image_map[] = {
    {"neutral", &emoji_1f636_64},
    {"happy", &emoji_1f642_64},
    {"laughing", &emoji_1f606_64},
    {"funny", &emoji_1f602_64},
    {"sad", &emoji_1f614_64},
    {"angry", &emoji_1f620_64},
    {"crying", &emoji_1f62d_64},
    {"loving", &emoji_1f60d_64},
    {"embarrassed", &emoji_1f633_64},
    {"surprised", &emoji_1f62f_64},
    {"shocked", &emoji_1f631_64},
    {"thinking", &emoji_1f914_64},
    {"winking", &emoji_1f609_64},
    {"cool", &emoji_1f60e_64},
    {"relaxed", &emoji_1f60c_64},
    {"delicious", &emoji_1f924_64},
    {"kissy", &emoji_1f618_64},
    {"confident", &emoji_1f60f_64},
    {"sleepy", &emoji_1f634_64},
    {"silly", &emoji_1f61c_64},
    {"confused", &emoji_1f644_64},
};

// 静态emoji图片对象
static lv_obj_t *s_emoji_img = NULL;

/**
 * @brief 根据名称查找并设置emoji图片
 * @param name 表情名称（如"happy", "sad"等）
 * @note 未知表情名称时默认显示neutral
 */
static void emoji_set_by_name(const char *name)
{
    if (!s_emoji_img || !name)
        return;

    const lv_image_dsc_t *img = &emoji_1f636_64; // 默认neutral

    for (int i = 0; i < sizeof(s_emoji_image_map) / sizeof(s_emoji_image_map[0]); i++)
    {
        if (strcmp(name, s_emoji_image_map[i].name) == 0)
        {
            img = s_emoji_image_map[i].image;
            break;
        }
    }

    lv_img_set_src(s_emoji_img, img);
}

/**
 * @brief 创建emoji显示对象
 * @param parent 父对象
 * @return 创建的emoji图片对象
 */
static lv_obj_t *create_emoji_display(lv_obj_t *parent)
{
    s_emoji_img = lv_img_create(parent);
    lv_obj_align(s_emoji_img, LV_ALIGN_CENTER, 0, -15);
    lv_img_set_src(s_emoji_img, &emoji_1f636_64); // 默认显示neutral
    return s_emoji_img;
}
#endif

extern lv_indev_t *indev_usr_btn;
static vw_ai_t vw = {0};

// 背景层对象
static lv_obj_t *s_bg_layer = NULL;      // 背景层（图片或VPG）
static lv_obj_t *s_overlay_layer = NULL; // 半透明遮罩层

LV_FONT_DECLARE(font_puhui_20_4)

LV_IMG_DECLARE(icon_mic)
LV_IMG_DECLARE(icon_speak)
LV_IMG_DECLARE(icon_connect)
LV_IMG_DECLARE(icon_bt_disconnect_18)
LV_IMG_DECLARE(icon_dhcp_dis)

void _on_eye_complate_vb(lv_anim_t *anim);

#ifdef CONFIG_USE_EYE_ANIMATION
// 眼睛动画模式：表情名称到动画索引的映射表
typedef struct
{
    uint8_t index;
    const char *text;
} _emoji_map_t;

static _emoji_map_t s_emoji_map[] = {
    {0, "neutral"},     // 静态状态/中性表情
    {2, "happy"},       // 开心表情
    {2, "laughing"},    // 开心表情
    {2, "funny"},       // 开心表情
    {1, "sad"},         // 悲伤表情
    {5, "angry"},       // 愤怒表情
    {1, "crying"},      // 悲伤表情
    {2, "loving"},      // 开心表情
    {3, "embarrassed"}, // 惊吓/惊讶表情
    {3, "surprised"},   // 惊吓/惊讶表情
    {3, "shocked"},     // 惊吓/惊讶表情
    {4, "thinking"},    // 不学/困惑表情
    {2, "winking"},     // 开心表情
    {2, "cool"},        // 开心表情
    {0, "relaxed"},     // 静态状态/中性表情
    {2, "delicious"},   // 开心表情
    {2, "kissy"},       // 开心表情
    {2, "confident"},   // 开心表情
    {0, "sleepy"},      // 静态状态/中性表情
    {2, "silly"},       // 开心表情
    {4, "confused"}     // 不学/困惑表情
};
#endif

static void clock_time_cb(lv_timer_t *timer)
{
    if (!vw.is_act)
    {
        return;
    }
    uint32_t now = ui_tool_get_timestamp_ms();
    if (now - vw.last_act >= 120 * 1000 && ui_tool_is_device_idle())
    {
        vw.last_act = now;
        page_change("home");
    }
}

static void _on_chat_msg(lv_event_t *e)
{
    // 1. 先检查页面是否激活
    if (!vw.is_act)
    {
        return;
    }

    // 2. 获取参数
    const char *msg = lv_event_get_param(e);
    lv_obj_t *label = lv_event_get_user_data(e);

    // 3. 验证消息内容
    if (!msg)
    {
        return;
    }

    // 4. 验证 label 对象
    if (!label || !lv_obj_is_valid(label))
    {
        return;
    }

    // 5. 检查 label 的父对象是否有效（防止对象已被删除但内存未释放）
    lv_obj_t *parent = lv_obj_get_parent(label);
    if (!parent || !lv_obj_is_valid(parent))
    {
        return;
    }

    // 6. 更新时间戳
    vw.last_act = ui_tool_get_timestamp_ms();

    // 7. 安全地设置文本
    lv_label_set_text(label, msg);
}

static void _on_status_evt(lv_event_t *e)
{
    // 1. 先检查页面是否激活
    if (!vw.is_act)
    {
        return;
    }

    // 2. 获取参数并验证
    const cl_ui_status_code_t *code = (cl_ui_status_code_t *)lv_event_get_param(e);
    lv_obj_t *icon_label = lv_event_get_user_data(e);

    if (!code || !icon_label || !lv_obj_is_valid(icon_label))
    {
        return;
    }

    vw.last_act = ui_tool_get_timestamp_ms();
    switch (*code)
    {
    case CL_UI_STA_NETWORK_DISCONNECT:
        lv_img_set_src(icon_label, &icon_dhcp_dis);
        if (lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case CL_UI_STA_BT_DISCONNECT:
        lv_img_set_src(icon_label, &icon_bt_disconnect_18);
        if (lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case CL_UI_STA_IDLE:
        if (!lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_add_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case CL_UI_STA_CONNECTTING:
        lv_img_set_src(icon_label, &icon_connect);
        if (lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        // cl_ui_send_event(CL_UI_EVENT_CHAT_MSG, "");
        break;
    case CL_UI_STA_LISTENING:
        lv_img_set_src(icon_label, &icon_mic);
        if (lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    case CL_UI_STA_SPEAKING:
        lv_img_set_src(icon_label, &icon_speak);
        if (lv_obj_has_flag(icon_label, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_clear_flag(icon_label, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    default:
        break;
    }
}

static void _on_emoji_evt(lv_event_t *e)
{
    // 1. 先检查页面是否激活
    if (!vw.is_act)
    {
        return;
    }

    // 2. 获取参数并验证
    const char *emotion = (char *)lv_event_get_param(e);
    if (!emotion)
    {
        return;
    }

#ifdef CONFIG_USE_EYE_ANIMATION
    // 眼睛动画模式：查找映射索引并设置动画
    uint8_t index = 0;
    for (uint8_t i = 0; i < sizeof(s_emoji_map) / sizeof(s_emoji_map[0]); i++)
    {
        if (strcmp(emotion, s_emoji_map[i].text) == 0)
        {
            index = s_emoji_map[i].index;
            break;
        }
    }
    eye_set_emotion(index);
#else
    // Emoji显示模式：直接根据名称设置emoji图片
    emoji_set_by_name(emotion);
#endif
}

// 按键事件处理
// static void _on_btn_cb(lv_event_t *e)
// {
//     if (!vw.is_act)
//         return;
//     cl_button_t *btn = (cl_button_t *)lv_event_get_param(e);
//     if (!btn)
//         return;
//     if (btn->id == USER_BUTTON_CENTER)
//     {
//         if (btn->event == BTN_DOUBLE_CLICK)
//         {
//             if (xiaozhi_status_get() == kDeviceStateConnecting || xiaozhi_status_get() == kDeviceStateConnectingW)
//             {
//                 return;
//             }
//             xiaozhi_stop_listen();
//             page_change("home");
//         }
//         else if (btn->event == BTN_CLICK)
//         {
//             else
//             {
//                 xiaozhi_toggle_chat_state();
//             }
//         }
//     }
//     else if (btn->id == USER_BUTTON_BACK && btn->event == BTN_CLICK)
//     {
//         if (xiaozhi_status_get() == kDeviceStateConnecting || xiaozhi_status_get() == kDeviceStateConnectingW)
//         {
//             return;
//         }
//         xiaozhi_stop_listen();
//         page_change("home");
//     }
// }

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

/**
 * @brief 清理背景层资源
 * @note 删除 LVGL 对象并清空引用
 */
static void _cleanup_background_layer(void)
{
    // 检查并删除背景层
    if (s_bg_layer != NULL)
    {
        lv_obj_del(s_bg_layer);
        s_bg_layer = NULL;
    }

    // 检查并删除遮罩层
    if (s_overlay_layer != NULL)
    {
        lv_obj_del(s_overlay_layer);
        s_overlay_layer = NULL;
    }
}

/**
 * @brief 加载聊天背景
 * @param root 父对象
 * @note 创建背景层和半透明遮罩层
 */
static void _load_chat_background(lv_obj_t *root)
{
    // 首先清理旧的背景层
    _cleanup_background_layer();

    // 先验证背景设置是否有效
    if (!chat_background_validate())
    {
        ESP_LOGI(TAG, "Background invalid or not set, using default black background");
        // 无效背景，不创建背景层，使用默认黑色背景
        return;
    }

    chat_bg_type_t bg_type = chat_background_get_type();
    const char *bg_path = chat_background_get_path();

    // 无背景设置时使用默认黑色背景
    if (bg_type == CHAT_BG_TYPE_NONE || bg_path == NULL)
    {
        ESP_LOGI(TAG, "No background set, using default black background");
        return;
    }

    ESP_LOGI(TAG, "Loading chat background: type=%d, path=%s", bg_type, bg_path);

    // 转换为LVGL路径格式: /sdcard/photo/01.jpg -> P:/photo/01.jpg
    static char lvgl_path[128];
    if (strncmp(bg_path, "/sdcard/", 8) == 0)
    {
        snprintf(lvgl_path, sizeof(lvgl_path), "P:%s", bg_path + 7);
    }
    else
    {
        strncpy(lvgl_path, bg_path, sizeof(lvgl_path) - 1);
        lvgl_path[sizeof(lvgl_path) - 1] = '\0';
    }

    ESP_LOGI(TAG, "LVGL path: %s", lvgl_path);

    // 根据背景类型创建对应的背景层
    if (bg_type == CHAT_BG_TYPE_IMAGE)
    {
        // 图片背景 - 使用缓存机制
        // const void *img_dsc = NULL;
        const lv_img_dsc_t *img_dsc = NULL;
        if (chat_background_load_to_cache(lvgl_path, (void **)&img_dsc))
        {
            // 缓存加载成功，使用内存描述符
            s_bg_layer = lv_img_create(root);
            lv_obj_set_size(s_bg_layer, LV_HOR_RES, LV_VER_RES);
            lv_obj_align(s_bg_layer, LV_ALIGN_CENTER, 0, 0);
            lv_img_set_src(s_bg_layer, img_dsc);
            // 设置图片缩放适配屏幕
            lv_img_set_zoom(s_bg_layer, 256); // 256 = 1:1 缩放
            ESP_LOGI(TAG, "Image background loaded from cache");
        }
        else
        {
            // 缓存加载失败，回退到文件路径方式
            ESP_LOGW(TAG, "Failed to load image to cache, falling back to file path");
            s_bg_layer = lv_img_create(root);
            lv_obj_set_size(s_bg_layer, LV_HOR_RES, LV_VER_RES);
            lv_obj_align(s_bg_layer, LV_ALIGN_CENTER, 0, 0);
            lv_img_set_src(s_bg_layer, lvgl_path);
            lv_img_set_zoom(s_bg_layer, 256);
            ESP_LOGI(TAG, "Image background loaded from file");
        }
    }
    else if (bg_type == CHAT_BG_TYPE_VPG)
    {
        // VPG动画背景
        s_bg_layer = lv_vpg_create(root);
        lv_obj_set_size(s_bg_layer, LV_HOR_RES, LV_VER_RES);
        lv_obj_align(s_bg_layer, LV_ALIGN_CENTER, 0, 0);
        lv_vpg_set_src(s_bg_layer, lvgl_path);
        ESP_LOGI(TAG, "VPG animation background loaded");
    }

    // 确保背景层在最底层
    if (s_bg_layer)
    {
        lv_obj_move_background(s_bg_layer);
    }

    // 不创建遮罩层，让背景完全显示
    s_overlay_layer = NULL;

    ESP_LOGI(TAG, "Chat background created successfully");
}

vw_ai_t *ai_view_create(lv_obj_t *root)
{

    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // 加载聊天背景（在最开头创建，确保在最底层）
    _load_chat_background(root);

    lv_obj_t *speak_staus = lv_img_create(root);
    lv_obj_align(speak_staus, LV_ALIGN_TOP_MID, 0, 10);

    int status = ui_tool_get_device_state();
    switch (status)
    {
    case kDeviceStateSpeaking:
        lv_img_set_src(speak_staus, &icon_speak);
        break;
    case kDeviceStateListening:
        lv_img_set_src(speak_staus, &icon_mic);
        break;
    case kDeviceStateConnecting:
        lv_img_set_src(speak_staus, &icon_connect);
        break;
        // default:
        //     if (get_bt_connect_status() < BT_STATUS_CONNECTING)
        //     {
        //         lv_img_set_src(speak_staus, &icon_bt_disconnect_18);
        //     }
        //     else
        //     {
        //         if (!net_get_dhcp_flag())
        //         {
        //             lv_img_set_src(speak_staus, &icon_dhcp_dis);
        //         }
        //     }
        //     // lv_obj_add_flag(speak_staus, LV_OBJ_FLAG_HIDDEN);
        //     break;
    }

#ifdef CONFIG_USE_EYE_ANIMATION
    lv_obj_t *eye = create_emotion_eye(root);
#else
    lv_obj_t *emoji = create_emoji_display(root);
#endif

    lv_obj_t *msg_label = lv_label_create(root);
    lv_obj_set_style_max_width(msg_label, 200, 0);
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_MODE_SCROLL);
    lv_obj_set_style_text_color(msg_label, lv_color_white(), 0);
    lv_obj_align(msg_label, LV_ALIGN_BOTTOM_MID, 0, -40);
    // 使用默认字体，不设置自定义字体避免多线程冲突
    lv_label_set_text(msg_label, "");

    // ctrl_center_create(root, NULL);

    vw.is_act = 0;
    lv_obj_add_event_cb(root, _on_chat_msg, CL_UI_EVENT_CHAT_MSG, msg_label);
    lv_obj_add_event_cb(root, _on_status_evt, CL_UI_EVENT_CHAT_STATUS, speak_staus);
    lv_obj_add_event_cb(root, _on_emoji_evt, CL_UI_EVENT_CHAT_EMOJI, NULL);

    vw.last_act = ui_tool_get_timestamp_ms();
    vw.speak_staus = speak_staus;
    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    static uint8_t first_flag = 1;
    if (first_flag)
    {
        first_flag = 0;
        lv_timer_create(clock_time_cb, 400, NULL);
    }

    return &vw;
}

/**
 * @brief AI 页面销毁回调
 * @param root 页面根对象
 * @note 在页面销毁时清理背景资源
 */
void ai_view_destroy(lv_obj_t *root)
{
    // 清理背景层
    _cleanup_background_layer();

    // 清除图片缓存
    chat_background_clear_cache();

    // 重置页面激活状态
    vw.is_act = 0;

    ESP_LOGI(TAG, "AI view destroyed, resources cleaned up");
}