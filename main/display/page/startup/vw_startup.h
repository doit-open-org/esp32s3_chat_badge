#ifndef __STARTUP_VIEW_H__
#define __STARTUP_VIEW_H__

#include "lvgl.h"

typedef struct
{
    uint8_t is_act; /* 如果没有 u8，改成 uint8_t 并 #include <stdint.h> */
} startup_view_t;

startup_view_t* startup_view_create(lv_obj_t *root);
void startup_view_delete(void);

#endif /* __STARTUP_VIEW_H__ */
