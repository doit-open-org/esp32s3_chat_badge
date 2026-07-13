#ifndef _CL_UI_H_
#define _CL_UI_H_

#include "page_manager/inc/page_manager.h"
#include "cl_ui.h"
#include "assert/language.h"
#include <string.h>
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SIMULATOR
#include "esp_log.h"
#else
typedef enum
{
    USER_BUTTON_CENTER,
    USER_BUTTON_BACK,
} cl_ui_btn_t;
typedef enum
{
    BTN_DOWN,
    BTN_CLICK,
    BTN_DOUBLE_CLICK,
    BTN_REPEAT_CLICK,
    BTN_SHORT_START,
    BTN_SHORT_UP,
    BTN_LONG_START,
    BTN_LONG_UP,
    BTN_LONG_HOLD,
    BTN_LONG_HOLD_UP,
    BTN_MAX,
    BTN_NONE,
} cl_key_event_t;
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
#define CL_UI_EVENT_BTN CL_UI_EVENT_BUTTON
#define ESP_LOGI(tag, format, ...) printf("\033[0;32m" format "\033[0m\n", ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("\033[0;33m" format "\033[0m\n", ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("\033[0;31m" format "\033[0m\n", ##__VA_ARGS__)
#define lvgl_port_unlock()
#define lvgl_port_lock(a)
#endif

    typedef enum
    {
        CL_UI_EVENT_MUSIC_STATUS = LV_EVENT_LAST + 1,
        CL_UI_EVENT_MODE_CHANGE,
        CL_UI_EVENT_BT_STATUS,
        CL_UI_EVENT_MUSIC_TITLE,
        CL_UI_EVENT_MUSIC_LYRC,
        CL_UI_EVENT_MUSIC_TIME,
        CL_UI_EVENT_MUSIC_IDX,
        CL_UI_EVENT_BUTTON,
        CL_UI_EVENT_BAT_CHANGE,   // 电池电量变化事件
        CL_UI_EVENT_CHARG_CHANGE, // 充电状态变化事件
        CL_UI_EVENT_CHAT_MSG,     // AI 聊天消息事件
        CL_UI_EVENT_CHAT_STATUS,  // AI 聊天状态事件
        CL_UI_EVENT_CHAT_EMOJI,   // AI 表情事件
    } cl_ui_event_t;              //

    typedef enum
    {
        CL_UI_STA_IDLE,               // 空闲状态
        CL_UI_STA_CONNECTTING,        // 连接中
        CL_UI_STA_LISTENING,          // 监听中
        CL_UI_STA_SPEAKING,           // 说话中
        CL_UI_STA_NETWORK_DISCONNECT, // 网络断开
        CL_UI_STA_BT_DISCONNECT,      // 蓝牙断开
        CL_UI_STA_BT_CONNECTED,       // 蓝牙已连接
    } cl_ui_status_code_t;

    typedef enum
    {
        CL_UI_KEY_POWER,    //
        CL_UI_KEY_MODE,     //
        CL_UI_KEY_VOL_UP,   //
        CL_UI_KEY_VOL_DOWN, //
    } cl_ui_key_t;

    typedef enum
    {
        CL_BTN_DOWN,
        CL_BTN_CLICK,
        CL_BTN_DOUBLE_CLICK,
        CL_BTN_REPEAT_CLICK,
        CL_BTN_SHORT_START,
        CL_BTN_SHORT_UP,
        CL_BTN_LONG_START,
        CL_BTN_LONG_UP,
        CL_BTN_LONG_HOLD,
        CL_BTN_LONG_HOLD_UP,
        CL_BTN_MAX,
        CL_BTN_NONE,
    } cl_ui_key_event_t;

    typedef struct
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
    } cl_time_t;

    typedef struct
    {
        uint16_t scan_cnt;  //
        uint16_t click_cnt; //
        cl_ui_key_t id;     //
        cl_ui_key_event_t event;
    } cl_button_t;

    void ui_init();
    int page_change(const char *name);
    lv_obj_t *ui_get_home();
    void ui_key_msg(cl_button_t *e, void *arg);
    void wakeup_ai();
    void xz_ui_evt_send(cl_ui_event_t code, void *param, uint32_t len);
    const char *get_current_page_name(void);

    // AI 事件发送便捷函数
    static inline void cl_ui_send_event(cl_ui_event_t code, const void *param)
    {
        if (param == NULL)
        {
            xz_ui_evt_send(code, NULL, 0);
        }
        else if (code == CL_UI_EVENT_CHAT_MSG || code == CL_UI_EVENT_CHAT_EMOJI)
        {
            // 字符串参数
            xz_ui_evt_send(code, (void *)param, strlen((const char *)param) + 1);
        }
        else if (code == CL_UI_EVENT_CHAT_STATUS)
        {
            // 状态码参数
            xz_ui_evt_send(code, (void *)param, sizeof(cl_ui_status_code_t));
        }
        else
        {
            xz_ui_evt_send(code, (void *)param, sizeof(void *));
        }
    }

#ifdef __cplusplus
}
#endif

#endif