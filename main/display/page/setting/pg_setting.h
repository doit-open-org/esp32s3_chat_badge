#ifndef SETTING_H
#define SETTING_H
#include "page_manager.h"
#include "vm_setting.h"
typedef struct { 
    page_base_t base;
    const char *name; 
    setting_view_t *vw;
    page_vtable_t *page_param; 
} setting_t;

page_vtable_t* setting_create(const char *name);

#endif
