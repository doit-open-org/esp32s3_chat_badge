#include "alarm_api.h"
#include "xiaozhi_alarm.h"
#include "ui_tool.h"
#include "cl_ui_event.h"
#include "cl_ui.h"
#include <esp_log.h>
#include <string.h>

#define TAG "alarm_service"

// 闹钟响铃配置
#define ALARM_RING_INTERVAL_MS 2000 // 每5秒播放一次铃声
#define ALARM_RING_MAX_COUNT 20     // 最多播放60次（5分钟）

extern int page_change(const char *name);

#define LV_EVENT_ALARM_TRIGGER ((lv_event_code_t)(_LV_EVENT_LAST + 100))

typedef struct
{
    u8 alarm_index;
} alarm_trigger_param_t;

// 响铃状态
static lv_timer_t *g_alarm_ring_timer = NULL;
static u8 g_alarm_ring_count = 0;

// 延迟页面切换的定时器回调
static void delayed_page_change_cb(lv_timer_t *timer)
{
    page_change("alarm_ring");
    lv_timer_del(timer);
}

// 循环播放铃声的定时器回调
static void alarm_ring_repeat_cb(lv_timer_t *timer)
{
    g_alarm_ring_count++;

    if (g_alarm_ring_count >= ALARM_RING_MAX_COUNT)
    {
        if (g_alarm_ring_timer)
        {
            lv_timer_del(g_alarm_ring_timer);
            g_alarm_ring_timer = NULL;
        }
        g_alarm_ring_count = 0;

        xiaozhi_alarm_stop();
        return;
    }

    ui_tool_play_alarm_sound();
}

// 停止响铃
void alarm_service_stop_ring(void)
{
    if (g_alarm_ring_timer)
    {
        lv_timer_del(g_alarm_ring_timer);
        g_alarm_ring_timer = NULL;
    }
    g_alarm_ring_count = 0;
}

// 事件路由回调
static void alarm_trigger_event_cb(lv_event_code_t code, void *param, uint32_t param_len, void *user_ctx)
{
    if (code != LV_EVENT_ALARM_TRIGGER || !param)
    {
        return;
    }

    alarm_trigger_param_t *trigger_param = (alarm_trigger_param_t *)param;
    u8 index = trigger_param->alarm_index;


    // 1. 调用业务层的响铃函数
    xiaozhi_alarm_ring(index);

    // 2. 播放第一次铃声
    ui_tool_play_alarm_sound();

    // 3. 启动循环播放定时器
    if (g_alarm_ring_timer)
    {
        lv_timer_del(g_alarm_ring_timer);
    }
    g_alarm_ring_count = 1; 
    g_alarm_ring_timer = lv_timer_create(alarm_ring_repeat_cb, ALARM_RING_INTERVAL_MS, NULL);

    // 4. 延迟切换到闹钟页面
    lv_timer_t *page_timer = lv_timer_create(delayed_page_change_cb, 50, NULL);
    lv_timer_set_repeat_count(page_timer, 1); 
}

// 闹钟触发处理函数
static void alarm_trigger_handler(u8 index)
{
    alarm_trigger_param_t param = {
        .alarm_index = index};

    // 异步发送事件到GUI线程处理
    esp_err_t ret = lv_event_system_send_async(
        LV_EVENT_ALARM_TRIGGER,
        &param,
        sizeof(param),
        alarm_trigger_event_cb,
        NULL);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send alarm trigger event: %d", ret);
    }
}

void alarm_service_init(void)
{
    // 初始化底层API
    alarm_init();

    // 注册触发回调
    alarm_set_trigger_callback(alarm_trigger_handler);
}

void alarm_service_deinit(void)
{
    alarm_deinit();
}
