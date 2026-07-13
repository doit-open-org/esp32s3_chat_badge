#ifndef UI_TOOL_H
#define UI_TOOL_H

#include <stdint.h>
#include "cl_ui.h"
#ifdef __cplusplus
extern "C" {
#endif

// 自动关机时间相关
uint32_t ui_tool_get_sleep_time(void);
void ui_tool_set_sleep_time(uint32_t time_sec);

// 亮度控制相关
uint8_t ui_tool_get_brightness(void);
void ui_tool_set_brightness(uint8_t brightness);

// 音量控制相关
int ui_tool_get_volume(void);
void ui_tool_set_volume(int volume);

// 电量相关
uint8_t ui_tool_get_battery_percent(void);
uint8_t ui_tool_get_charging_state(void);

// 设备状态相关
int ui_tool_get_device_state(void);
uint8_t ui_tool_is_device_idle(void);

// 时间戳相关
uint32_t ui_tool_get_timestamp_ms(void);

cl_time_t ui_tool_get_time(void);

// 音频播放相关（用于闹钟铃声等）
void ui_tool_play_alarm_sound(void);

// WiFi 状态相关
uint8_t ui_tool_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif

#endif // UI_TOOL_H
