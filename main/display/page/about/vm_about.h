#ifndef ABOUT_VIEW_H
#define ABOUT_VIEW_H
#include "lvgl.h"

typedef struct {
    uint8_t is_act;
    lv_obj_t *info_cont;
} about_view_t;

about_view_t* about_view_create(lv_obj_t *root);
void about_view_delete(void);

#endif
