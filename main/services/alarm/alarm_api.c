#include "alarm_api.h"
#include <string.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "alarm_api"

// NVS配置
#define ALARM_NVS_NAMESPACE "alarm"
#define ALARM_NVS_KEY_FMT "alarm_%d"
#define ALARM_COUNT_KEY "count"
#define ALARM_MAP_KEY "map"

// 闹钟映射表
typedef struct
{
    u32 map;        // 哪些索引有闹钟 (bit0-4)
    u32 map_sw;     // 哪些闹钟是开启的 (bit0-4)
    u32 active_map; // 当前激活的闹钟 (bit0-4)
} alarm_map_t;

// 全局变量
static alarm_t alarm_tab[MAX_ALARM_NUMS] = {0};
static alarm_map_t alarm_map = {0};
static volatile u8 g_alarm_active_flag = 0;
static volatile u8 alarm_cur_active = 0;
static esp_timer_handle_t alarm_timer = NULL;
static bool alarm_initialized = false;

// 闹钟触发回调函数
static void (*alarm_trigger_callback)(u8 index) = NULL;

// ==================== NVS存储函数 ====================

static void alarm_nvs_save_map(void)
{
    nvs_handle_t handle;
    if (nvs_open(ALARM_NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_blob(handle, ALARM_MAP_KEY, &alarm_map, sizeof(alarm_map_t));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static void alarm_nvs_load_map(void)
{
    nvs_handle_t handle;
    size_t size = sizeof(alarm_map_t);

    if (nvs_open(ALARM_NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK)
    {
        if (nvs_get_blob(handle, ALARM_MAP_KEY, &alarm_map, &size) != ESP_OK)
        {
            memset(&alarm_map, 0, sizeof(alarm_map_t));
        }
        nvs_close(handle);
    }
}

static void alarm_nvs_save_info(u8 index)
{
    if (index >= MAX_ALARM_NUMS)
        return;

    nvs_handle_t handle;
    char key[16];
    snprintf(key, sizeof(key), ALARM_NVS_KEY_FMT, index);

    if (nvs_open(ALARM_NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_blob(handle, key, &alarm_tab[index], sizeof(alarm_t));
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static void alarm_nvs_load_info(u8 index)
{
    if (index >= MAX_ALARM_NUMS)
        return;

    nvs_handle_t handle;
    char key[16];
    size_t size = sizeof(alarm_t);
    snprintf(key, sizeof(key), ALARM_NVS_KEY_FMT, index);

    if (nvs_open(ALARM_NVS_NAMESPACE, NVS_READONLY, &handle) == ESP_OK)
    {
        if (nvs_get_blob(handle, key, &alarm_tab[index], &size) != ESP_OK)
        {
            memset(&alarm_tab[index], 0, sizeof(alarm_t));
        }
        nvs_close(handle);
    }
}

static void alarm_nvs_load_all(void)
{
    alarm_nvs_load_map();

    for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.map & (1 << i))
        {
            alarm_nvs_load_info(i);
        }
    }
}

// ==================== 时间工具函数 ====================

u16 month_for_day(u8 month, u16 year)
{
    const u8 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12)
        return 30;

    u16 day = days[month - 1];

    // 闰年2月
    if (month == 2)
    {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        {
            day = 29;
        }
    }

    return day;
}

void rtc_calculate_next_few_day(sys_time_t *data_time, u8 days)
{
    if (!days || days >= 29)
        return;

    u16 tmp16 = month_for_day(data_time->month, data_time->year);
    data_time->day += days;

    if (data_time->day > tmp16)
    {
        data_time->month++;
        data_time->day -= tmp16;

        if (data_time->month > 12)
        {
            data_time->month = 1;
            data_time->year++;
        }
    }
}

u8 rtc_calculate_week_val(sys_time_t *data_time)
{
    sys_time_t t_time = *data_time;
    u32 century, val, year;

    if (t_time.month < 3)
    {
        t_time.month += 12;
        t_time.year--;
    }

    year = t_time.year % 100;
    century = t_time.year / 100;
    val = year + (year / 4) + (century / 4) + (26 * (t_time.month + 1) / 10) + t_time.day;
    val = val - century * 2 - 1;

    return (u8)(val % 7);
}

static void get_current_time(sys_time_t *sys_time)
{
    time_t now = time(NULL);

    // // 检查时间是否已同步（时间戳应该大于2020年）
    // if (now < 1577836800)
    // { // 2020-01-01 00:00:00
    //     ESP_LOGW(TAG, "System time not synced yet, using default time");
    //     sys_time->year = 2024;
    //     sys_time->month = 1;
    //     sys_time->day = 1;
    //     sys_time->hour = 0;
    //     sys_time->min = 0;
    //     sys_time->sec = 0;
    //     return;
    // }

    struct tm *timeinfo = localtime(&now);

    sys_time->year = timeinfo->tm_year + 1900;
    sys_time->month = timeinfo->tm_mon + 1;
    sys_time->day = timeinfo->tm_mday;
    sys_time->hour = timeinfo->tm_hour;
    sys_time->min = timeinfo->tm_min;
    sys_time->sec = timeinfo->tm_sec;

    // 检查时间是否已同步
    if (now < 1577836800)
    { 
        ESP_LOGW(TAG, "System time not synced, alarms will work with system time (may be inaccurate)");
        
    }
}

// ==================== 时间计算引擎 ====================

static void alarm_calc_time_by_week_mode(sys_time_t *pTime, u8 mode)
{
    u8 alarm_week = rtc_calculate_week_val(pTime);
    if (alarm_week == 0)
        alarm_week = 7;

    u8 tmp_mode = 0;

    // 查找当前可以设置闹钟日期最近的日期
    for (u8 i = 1; i < 8; i++)
    {
        if (mode & (1 << i))
        {
            if (i >= alarm_week)
            {
                tmp_mode = i;
                break;
            }
        }
    }

    // 如果没找到，从下周开始找
    if (tmp_mode == 0)
    {
        for (u8 i = 1; i < 8; i++)
        {
            if (mode & (1 << i))
            {
                tmp_mode = i;
                break;
            }
        }
    }

    if (tmp_mode >= 1 && tmp_mode < 8)
    {
        if (tmp_mode > alarm_week)
        {
            rtc_calculate_next_few_day(pTime, tmp_mode - alarm_week);
        }
        else if (tmp_mode < alarm_week)
        {
            rtc_calculate_next_few_day(pTime, 7 - (alarm_week - tmp_mode));
        }
    }
}

static void alarm_calc_real_time_by_index(sys_time_t *cTime, u8 index)
{
    if (index >= MAX_ALARM_NUMS)
        return;

    alarm_t *pAlarm = &alarm_tab[index];
    sys_time_t *pTime = &pAlarm->time;

    if (pAlarm->mode > 0xFE)
        return;

    u32 c_tmp = ((cTime->hour & 0x1f) << 12) | ((cTime->min & 0x3f) << 6) | (cTime->sec & 0x3f);
    u32 p_tmp = ((pTime->hour & 0x1f) << 12) | ((pTime->min & 0x3f) << 6) | (pTime->sec & 0x3f);

    if (p_tmp > c_tmp)
    {
        // 时间未到，设为今天
        pTime->year = cTime->year;
        pTime->month = cTime->month;
        pTime->day = cTime->day;
        pTime->sec = 0;
    }
    else
    {
        // 时间已过，设为明天
        sys_time_t tmp = *cTime;
        rtc_calculate_next_few_day(&tmp, 1);
        pTime->year = tmp.year;
        pTime->month = tmp.month;
        pTime->day = tmp.day;
        pTime->sec = 0;
    }

    // 处理星期模式
    if (pAlarm->mode != ALARM_MODE_ONCE && pAlarm->mode != ALARM_MODE_EVERY_DAY)
    {
        alarm_calc_time_by_week_mode(pTime, pAlarm->mode);
    }
}

static void alarm_update_all_time(sys_time_t *cTime)
{
    for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.map_sw & (1 << i))
        {
            alarm_calc_real_time_by_index(cTime, i);
        }
    }
}

// ==================== 定时器管理 ====================

static int alarm_cmp_time(sys_time_t *time1, sys_time_t *time2)
{
    if (time1->year != time2->year)
        return time1->year > time2->year ? 1 : -1;
    if (time1->month != time2->month)
        return time1->month > time2->month ? 1 : -1;
    if (time1->day != time2->day)
        return time1->day > time2->day ? 1 : -1;
    if (time1->hour != time2->hour)
        return time1->hour > time2->hour ? 1 : -1;
    if (time1->min != time2->min)
        return time1->min > time2->min ? 1 : -1;
    if (time1->sec != time2->sec)
        return time1->sec > time2->sec ? 1 : -1;
    return 0;
}

static void alarm_timer_callback(void *arg)
{
    // 设置闹钟激活标志
    g_alarm_active_flag = 1;

    ESP_LOGI(TAG, "Alarm triggered!");

    // 调用触发回调函数
    if (alarm_trigger_callback)
    {
        // 找到触发的闹钟索引
        for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
        {
            if (alarm_map.active_map & (1 << i))
            {
                alarm_trigger_callback(i);
                break;
            }
        }
    }
}

static void alarm_get_the_earliest(void)
{
    u8 index = 0xFF;
    sys_time_t *pTmp = NULL;
    alarm_map.active_map = 0;

    // 找到第一个开启的闹钟
    for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.map_sw & (1 << i))
        {
            alarm_map.active_map |= (1 << i);
            pTmp = &alarm_tab[i].time;
            index = i;
            break;
        }
    }

    if (index == 0xFF)
    {
        // 没有闹钟，停止定时器
        if (alarm_timer)
        {
            esp_timer_stop(alarm_timer);
        }
        return;
    }

    // 找到最早的闹钟
    for (u8 i = index + 1; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.map_sw & (1 << i))
        {
            int ret = alarm_cmp_time(pTmp, &alarm_tab[i].time);
            if (ret == 0)
            {
                alarm_map.active_map |= (1 << i);
            }
            else if (ret == 1)
            {
                pTmp = &alarm_tab[i].time;
                index = i;
                alarm_map.active_map = 0;
                alarm_map.active_map |= (1 << i);
            }
        }
    }

    // 计算距离触发的时间
    sys_time_t current_time;
    get_current_time(&current_time);

    struct tm alarm_tm = {0};
    alarm_tm.tm_year = pTmp->year - 1900;
    alarm_tm.tm_mon = pTmp->month - 1;
    alarm_tm.tm_mday = pTmp->day;
    alarm_tm.tm_hour = pTmp->hour;
    alarm_tm.tm_min = pTmp->min;
    alarm_tm.tm_sec = pTmp->sec;

    time_t alarm_time = mktime(&alarm_tm);
    time_t current = time(NULL);

    int64_t diff = (int64_t)(alarm_time - current);

    if (diff > 0)
    {
        // 停止旧定时器
        if (alarm_timer)
        {
            esp_timer_stop(alarm_timer);
        }

        // 启动新定时器
        uint64_t timeout_us = diff * 1000000ULL;
        esp_timer_start_once(alarm_timer, timeout_us);

        ESP_LOGI(TAG, "Next alarm in %d seconds at %04d-%02d-%02d %02d:%02d:%02d",
                 (int)diff, pTmp->year, pTmp->month, pTmp->day, pTmp->hour, pTmp->min, pTmp->sec);
    }
}

static void alarm_update(void)
{
    sys_time_t current_time;
    get_current_time(&current_time);

    alarm_update_all_time(&current_time);
    alarm_get_the_earliest();
}

// ==================== 公共API ====================

void alarm_init(void)
{
    if (alarm_initialized)
        return;

    ESP_LOGI(TAG, "Alarm init");

    // 检查系统时间是否已同步
    time_t now = time(NULL);
    if (now < 1577836800)
    { // 2020-01-01 00:00:00
        ESP_LOGW(TAG, "System time not synced (timestamp: %ld), alarm may not work correctly", (long)now);
    }

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // 创建定时器
    esp_timer_create_args_t timer_args = {
        .callback = alarm_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "alarm_timer"};
    esp_timer_create(&timer_args, &alarm_timer);

    // 从NVS加载闹钟数据
    alarm_nvs_load_all();

    // 更新闹钟时间并启动定时器
    if (!g_alarm_active_flag)
    {
        alarm_update();
    }

    alarm_initialized = true;
}

void alarm_deinit(void)
{
    if (!alarm_initialized)
        return;

    if (alarm_timer)
    {
        esp_timer_stop(alarm_timer);
        esp_timer_delete(alarm_timer);
        alarm_timer = NULL;
    }

    alarm_initialized = false;
}

u8 alarm_add(alarm_t *p, u8 index)
{
    if (!alarm_initialized)
    {
        ESP_LOGE(TAG, "Alarm not initialized");
        return 0;
    }

    if (index >= MAX_ALARM_NUMS)
    {
        ESP_LOGE(TAG, "Invalid alarm index: %d", index);
        return 0;
    }

    if (p->mode > 0xFE)
    {
        ESP_LOGE(TAG, "Invalid alarm mode: 0x%02x", p->mode);
        return 0;
    }

    // 更新内存中的闹钟
    alarm_map.map |= (1 << index);

    if (p->sw == 0)
    {
        alarm_map.map_sw &= ~(1 << p->index);
    }
    else
    {
        alarm_map.map_sw |= (1 << p->index);
    }

    alarm_tab[index].index = p->index;
    alarm_tab[index].sw = p->sw;
    alarm_tab[index].mode = p->mode;
    alarm_tab[index].time.hour = p->time.hour;
    alarm_tab[index].time.min = p->time.min;

    // 计算实际触发时间
    sys_time_t current_time;
    get_current_time(&current_time);
    alarm_calc_real_time_by_index(&current_time, index);

    // 保存到NVS
    alarm_nvs_save_info(index);
    alarm_nvs_save_map();

    // 更新定时器
    alarm_get_the_earliest();

    ESP_LOGI(TAG, "Alarm %d %s: %02d:%02d mode=0x%02x",
             index, p->sw ? "ON" : "OFF", p->time.hour, p->time.min, p->mode);

    return 1;
}

void alarm_delete(u8 index)
{
    if (!alarm_initialized)
        return;

    if (index >= MAX_ALARM_NUMS)
    {
        ESP_LOGE(TAG, "Invalid alarm index: %d", index);
        return;
    }

    ESP_LOGI(TAG, "Delete alarm %d", index);

    alarm_map.map &= ~(1 << index);
    alarm_map.map_sw &= ~(1 << index);
    alarm_tab[index].sw = 0;

    // 保存到NVS
    alarm_nvs_save_map();

    // 更新定时器
    alarm_get_the_earliest();
}

u8 alarm_get_info(alarm_t *p, u8 index)
{
    if (index >= MAX_ALARM_NUMS)
        return 0;

    if (alarm_map.map & (1 << index))
    {
        p->index = alarm_tab[index].index;
        p->sw = alarm_tab[index].sw;
        p->mode = alarm_tab[index].mode;
        p->time.hour = alarm_tab[index].time.hour;
        p->time.min = alarm_tab[index].time.min;
        return 1;
    }

    memset(p, 0, sizeof(alarm_t));
    return 0;
}

u8 alarm_get_total(void)
{
    u8 total = 0;

    for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.map & (1 << i))
        {
            total++;
        }
    }

    return total;
}

void alarm_update_time_api(sys_time_t *time)
{
    if (!alarm_initialized)
        return;

    alarm_update_all_time(time);
    alarm_get_the_earliest();
    alarm_nvs_save_map();
}

void alarm_update_info_after_trigger(void)
{
    if (!alarm_initialized)
        return;

    sys_time_t current_time;
    get_current_time(&current_time);

    alarm_cur_active = alarm_map.active_map;

    // 处理触发的闹钟
    for (u8 i = 0; i < MAX_ALARM_NUMS; i++)
    {
        if (alarm_map.active_map & (1 << i))
        {
            if (alarm_tab[i].mode != ALARM_MODE_ONCE)
            {
                // 重复闹钟，计算下次触发时间
                alarm_calc_real_time_by_index(&current_time, i);
            }
            else
            {
                // 单次闹钟，关闭
                alarm_map.map_sw &= ~(1 << i);
                alarm_tab[i].sw = 0;
                alarm_nvs_save_info(i);
            }
        }
    }

    // 保存并更新定时器
    alarm_nvs_save_map();
    alarm_get_the_earliest();
}

void alarm_active_flag_set(u8 flag)
{
    g_alarm_active_flag = flag;
}

u8 alarm_active_flag_get(void)
{
    return g_alarm_active_flag;
}

u8 alarm_get_active_index(void)
{
    return alarm_cur_active;
}

void alarm_set_trigger_callback(void (*callback)(u8 index))
{
    alarm_trigger_callback = callback;
}
