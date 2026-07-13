#ifndef HELP_VIEW_H
#define HELP_VIEW_H
#include "lvgl.h"

typedef struct {
    lv_obj_t *cont_list[3];
    uint8_t is_act;
    uint8_t current_page;
} help_view_t;

help_view_t* help_view_create(lv_obj_t *root);
void help_view_delete(void);

#endif
