#include "ui_tool.h"
#include "settings.h"
#include "board.h"
#include "backlight.h"
#include "audio/audio_codec.h"
#include "application.h"
#include "device_state.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <time.h>
#include <sys/time.h>
#include "assets/lang_config.h"
#define TAG "UiTool"

// NVS命名空间和键名
#define SETTINGS_NAMESPACE "system"
#define KEY_SLEEP_TIME "sleep_time"
// 默认值：0表示从不关机
#define DEFAULT_SLEEP_TIME 0

extern "C" bool blufi_wifi_sta_get_connect_status(void);
uint32_t ui_tool_get_sleep_time(void) {
    Settings settings(SETTINGS_NAMESPACE, false);
    int32_t time = settings.GetInt(KEY_SLEEP_TIME, DEFAULT_SLEEP_TIME);
    
    // 确保时间为非负数
    if (time < 0) {
        time = DEFAULT_SLEEP_TIME;
    }
    
    ESP_LOGD(TAG, "Get sleep time: %d seconds", (int)time);
    return (uint32_t)time;
}

void ui_tool_set_sleep_time(uint32_t time_sec) {
    // 读取当前值，如果相同则不写入
    uint32_t current_time = ui_tool_get_sleep_time();
    if (current_time == time_sec) {
        ESP_LOGD(TAG, "Sleep time unchanged: %u seconds", (unsigned int)time_sec);
        return;
    }
    
    // 写入新值
    Settings settings(SETTINGS_NAMESPACE, true);
    settings.SetInt(KEY_SLEEP_TIME, (int32_t)time_sec);
    ESP_LOGI(TAG, "Sleep time set to: %u seconds", (unsigned int)time_sec);
}

uint8_t ui_tool_get_brightness(void)
{
    Board &board = Board::GetInstance();
    Backlight *backlight = board.GetBacklight();

    if (backlight)
    {
        uint8_t brightness = backlight->brightness();
        return brightness;
    }
    return 50; // 默认值
}

void ui_tool_set_brightness(uint8_t brightness)
{
    Board &board = Board::GetInstance();
    Backlight *backlight = board.GetBacklight();

    if (backlight)
    {
        if (brightness > 100)
        {
            brightness = 100;
        }

        backlight->SetBrightness(brightness, true);
    }
}
int ui_tool_get_volume(void)
{
    Board &board = Board::GetInstance();
    AudioCodec *codec = board.GetAudioCodec();

    if (codec)
    {
        int volume = codec->output_volume();
        return volume;
    }
    return 50; // 默认值
}

void ui_tool_set_volume(int volume)
{
    Board &board = Board::GetInstance();
    AudioCodec *codec = board.GetAudioCodec();

    if (codec)
    {
        // 限制音量范围 0-100
        if (volume < 0)
        {
            volume = 0;
        }
        else if (volume > 100)
        {
            volume = 100;
        }

        codec->SetOutputVolume(volume);
    }
}

uint8_t ui_tool_get_battery_percent(void)
{
    Board &board = Board::GetInstance();
    int level = 0;
    bool charging = false;
    bool discharging = false;

    if (board.GetBatteryLevel(level, charging, discharging))
    {
        // 确保电量在 0-100 范围内
        if (level < 0)
        {
            level = 0;
        }
        else if (level > 100)
        {
            level = 100;
        }

        ESP_LOGD(TAG, "Battery level: %d%%", level);
        return (uint8_t)level;
    }

    ESP_LOGW(TAG, "Battery level not available, returning default 50%%");
    return 50; // 默认值
}

uint8_t ui_tool_get_charging_state(void)
{
    Board &board = Board::GetInstance();
    int level = 0;
    bool charging = false;
    bool discharging = false;

    if (board.GetBatteryLevel(level, charging, discharging))
    {
        ESP_LOGD(TAG, "Charging state: %s", charging ? "charging" : "not charging");
        return charging ? 1 : 0;
    }

    ESP_LOGW(TAG, "Charging state not available, returning default 0");
    return 0; // 默认未充电
}

int ui_tool_get_device_state(void)
{
    return (int)Application::GetInstance().GetDeviceState();
}

uint8_t ui_tool_is_device_idle(void)
{
    DeviceState state = Application::GetInstance().GetDeviceState();
    return (state == kDeviceStateIdle) ? 1 : 0;
}

uint32_t ui_tool_get_timestamp_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

cl_time_t ui_tool_get_time(void)
{
    cl_time_t result = {0};
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    if (tm_info)
    {
        result.year = tm_info->tm_year + 1900; // tm_year是从1900年开始的
        result.month = tm_info->tm_mon + 1;    // tm_mon是0-11，需要+1
        result.day = tm_info->tm_mday;         // 1-31
        result.hour = tm_info->tm_hour;        // 0-23
        result.min = tm_info->tm_min;          // 0-59
        result.sec = tm_info->tm_sec;          // 0-59

        ESP_LOGD(TAG, "Get time: %04d-%02d-%02d %02d:%02d:%02d",
                 result.year, result.month, result.day,
                 result.hour, result.min, result.sec);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to get local time, returning default");
    }

    return result;
}

void ui_tool_play_alarm_sound(void)
{
    Application &app = Application::GetInstance();
    app.PlaySound(Lang::Sounds::OGG_ALARM_RING);

    ESP_LOGI(TAG, "Playing alarm sound");
}

uint8_t ui_tool_is_wifi_connected(void)
{
    return blufi_wifi_sta_get_connect_status() ? 1 : 0;
}