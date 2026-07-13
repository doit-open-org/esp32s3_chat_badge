#ifndef SETTING_VIEW_H
#define SETTING_VIEW_H
#include "lvgl.h"

typedef struct { 
    int dummy;
    uint8_t is_act;
    lv_obj_t *cont_info;
    lv_obj_t *rst_dialog;
} setting_view_t;

setting_view_t* setting_view_create(lv_obj_t *root);
void setting_view_delete(void);

#endif
