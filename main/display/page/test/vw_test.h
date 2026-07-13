#ifndef __TEST_VIEW_H__
#define __TEST_VIEW_H__

#include "lvgl.h"

typedef struct
{
    uint8_t is_act; /* 如果没有 u8，改成 uint8_t 并 #include <stdint.h> */
} test_view_t;

test_view_t* test_view_create(lv_obj_t *root);
void test_view_delete(void);

#endif /* __TEST_VIEW_H__ */
