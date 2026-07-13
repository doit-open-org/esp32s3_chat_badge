#ifndef _XIAOZHI_ALARM_H_
#define _XIAOZHI_ALARM_H_

#include <stdint.h>
#include <stdbool.h>

// 类型定义
typedef uint8_t u8;

// 闹钟信息结构体
typedef struct
{
    u8 onoff;     // 开关状态: 0=关闭, 1=开启
    u8 index;     // 闹钟索引 (0-4)
    u8 hour;      // 小时 (0-23)
    u8 min;       // 分钟 (0-59)
    u8 week_mask; // 星期掩码: bit0=周一, bit6=周日, 0x7F=每天, 0x00=单次
} xz_alarm_info_t;

// 闹钟管理接口
u8 xiaozhi_alarm_get_num();
u8 xiaozhi_alarm_remove_all();
bool xiaozhi_get_alarm_info(xz_alarm_info_t *alarm, u8 *num);
u8 xiaozhi_alarm_set(xz_alarm_info_t *alarm_info, u8 index);
u8 xiaozhi_alarm_add(xz_alarm_info_t *alarm_info);
u8 xiaozhi_alarm_get_free_alarm_index();
u8 xiaozhi_alarm_del(u8 index);

// 闹钟触发接口
void xiaozhi_alarm_ring(u8 index);
void xiaozhi_alarm_stop();
void xiaozhi_alarm_snooze();

#endif