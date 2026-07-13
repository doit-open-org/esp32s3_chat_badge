#ifndef __ALARM_SERVICE_H__
#define __ALARM_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

void alarm_service_init(void);
void alarm_service_deinit(void);
void alarm_service_stop_ring(void);
#ifdef __cplusplus
}
#endif

#endif // __ALARM_SERVICE_H__
