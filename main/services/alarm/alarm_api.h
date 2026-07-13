#ifndef __ALARM_API_H__
#define __ALARM_API_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 最大闹钟数量
#define MAX_ALARM_NUMS 5

// 闹钟模式枚举
typedef enum {
    ALARM_MODE_ONCE            = 0x00,  // 单次
    ALARM_MODE_EVERY_DAY       = 0x01,  // 每天
    ALARM_MODE_EVERY_MONDAY    = 0x02,  // 周一
    ALARM_MODE_EVERY_TUESDAY   = 0x04,  // 周二
    ALARM_MODE_EVERY_WEDNESDAY = 0x08,  // 周三
    ALARM_MODE_EVERY_THURSDAY  = 0x10,  // 周四
    ALARM_MODE_EVERY_FRIDAY    = 0x20,  // 周五
    ALARM_MODE_EVERY_SATURDAY  = 0x40,  // 周六
    ALARM_MODE_EVERY_SUNDAY    = 0x80,  // 周日
} alarm_mode_t;

// 系统时间结构
typedef struct {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
} sys_time_t;

// 闹钟结构
typedef struct {
    u8 index;       // 索引 (0-4)
    u8 sw;          // 开关 (0=关, 1=开)
    u8 mode;        // 重复模式
    sys_time_t time; // 时间
} alarm_t;

// 初始化和管理
void alarm_init(void);
void alarm_deinit(void);

// CRUD操作
u8 alarm_add(alarm_t *alarm, u8 index);
void alarm_delete(u8 index);
u8 alarm_get_info(alarm_t *alarm, u8 index);
u8 alarm_get_total(void);

// 时间更新
void alarm_update_time_api(sys_time_t *time);

// 闹钟触发相关
void alarm_active_flag_set(u8 flag);
u8 alarm_active_flag_get(void);
u8 alarm_get_active_index(void);
void alarm_update_info_after_trigger(void);

// 工具函数
u8 rtc_calculate_week_val(sys_time_t *data_time);
void rtc_calculate_next_few_day(sys_time_t *data_time, u8 days);
u16 month_for_day(u8 month, u16 year);

// 设置触发回调
void alarm_set_trigger_callback(void (*callback)(u8 index));

#ifdef __cplusplus
}
#endif

#endif // __ALARM_API_H__
