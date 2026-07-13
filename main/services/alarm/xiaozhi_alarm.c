#include "xiaozhi_alarm.h"
#include "alarm_api.h"
#include <string.h>
#include <stdio.h>
#include <esp_log.h>

#define TAG "xiaozhi_alarm"
#define MAX_ALARMS 5

// 数据格式转换：UI层 → 底层API
static void xz_to_alarm(const xz_alarm_info_t *xz, alarm_t *alarm) {
    alarm->index = xz->index;
    alarm->sw = xz->onoff;
    alarm->time.hour = xz->hour;
    alarm->time.min = xz->min;
    
    // 转换week_mask到mode
    if (xz->week_mask == 0x00) {
        alarm->mode = ALARM_MODE_ONCE;  // 单次
    } else if (xz->week_mask == 0x7F) {
        alarm->mode = ALARM_MODE_EVERY_DAY;  // 每天
    } else {
        // 星期模式：week_mask左移1位
        alarm->mode = xz->week_mask << 1;
    }
}

// 数据格式转换：底层API → UI层
static void alarm_to_xz(const alarm_t *alarm, xz_alarm_info_t *xz) {
    xz->index = alarm->index;
    xz->onoff = alarm->sw;
    xz->hour = alarm->time.hour;
    xz->min = alarm->time.min;
    
    // 转换mode到week_mask
    if (alarm->mode == ALARM_MODE_ONCE) {
        xz->week_mask = 0x00;  // 单次
    } else if (alarm->mode == ALARM_MODE_EVERY_DAY) {
        xz->week_mask = 0x7F;  // 每天
    } else {
        // 星期模式：mode右移1位
        xz->week_mask = alarm->mode >> 1;
    }
}

// 获取闹钟数量
u8 xiaozhi_alarm_get_num() {
    return alarm_get_total();
}

// 获取所有闹钟信息
bool xiaozhi_get_alarm_info(xz_alarm_info_t *alarm, u8 *num) {
    if (!alarm || !num) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }
    
    u8 count = alarm_get_total();
    
    if (*num < count) {
        ESP_LOGE(TAG, "Buffer too small: %d < %d", *num, count);
        return false;
    }
    
    u8 get_num = 0;
    for (u8 i = 0; i < MAX_ALARMS && get_num < count; i++) {
        alarm_t t_alarm = {0};
        if (alarm_get_info(&t_alarm, i)) {
            alarm_to_xz(&t_alarm, &alarm[get_num]);
            get_num++;
        }
    }
    
    *num = get_num;
    return true;
}

// 设置闹钟信息
u8 xiaozhi_alarm_set(xz_alarm_info_t *alarm_info, u8 index) {
    if (!alarm_info || index >= MAX_ALARMS) {
        ESP_LOGE(TAG, "Invalid parameters");
        return 0;
    }
    
    alarm_t t_alarm = {0};
    
    // 检查闹钟是否存在
    if (!alarm_get_info(&t_alarm, index)) {
        ESP_LOGE(TAG, "Alarm %d not found", index);
        return 0;
    }
    
    // 转换并更新
    xz_to_alarm(alarm_info, &t_alarm);
    t_alarm.index = index;
    
    return alarm_add(&t_alarm, index);
}

// 添加新闹钟
u8 xiaozhi_alarm_add(xz_alarm_info_t *alarm_info) {
    if (!alarm_info) {
        ESP_LOGE(TAG, "Invalid parameters");
        return 0;
    }
    
    alarm_t t_alarm = {0};
    xz_to_alarm(alarm_info, &t_alarm);
    t_alarm.index = alarm_info->index;
    
    ESP_LOGI(TAG, "Add alarm %d: %02d:%02d mode=0x%02x %s",
             t_alarm.index, t_alarm.time.hour, t_alarm.time.min,
             t_alarm.mode, t_alarm.sw ? "ON" : "OFF");
    
    return alarm_add(&t_alarm, alarm_info->index);
}

// 获取空闲闹钟索引
u8 xiaozhi_alarm_get_free_alarm_index() {
    alarm_t t_alarm = {0};
    
    for (u8 i = 0; i < MAX_ALARMS; i++) {
        if (!alarm_get_info(&t_alarm, i)) {
            return i;
        }
    }
    
    return 0xFF; // 没有空闲索引
}

// 删除闹钟
u8 xiaozhi_alarm_del(u8 index) {
    if (index >= MAX_ALARMS) {
        ESP_LOGE(TAG, "Invalid index: %d", index);
        return 0;
    }
    
    ESP_LOGI(TAG, "Delete alarm %d", index);
    alarm_delete(index);
    return 1;
}

// 删除所有闹钟
u8 xiaozhi_alarm_remove_all() {
    ESP_LOGI(TAG, "Remove all alarms");
    
    for (u8 i = 0; i < MAX_ALARMS; i++) {
        alarm_t t_alarm = {0};
        if (alarm_get_info(&t_alarm, i)) {
            alarm_delete(i);
        }
    }
    
    return 1;
}

// 闹钟响铃
void xiaozhi_alarm_ring(u8 index) {
    ESP_LOGI(TAG, "Alarm %d ringing", index);
}

void xiaozhi_alarm_stop()
{
    extern void alarm_service_stop_ring(void);
    alarm_service_stop_ring();
    alarm_active_flag_set(0);
    alarm_update_info_after_trigger();
}

// 贪睡
void xiaozhi_alarm_snooze() {
    ESP_LOGI(TAG, "Snooze alarm");
    // TODO: 实现贪睡逻辑
}
