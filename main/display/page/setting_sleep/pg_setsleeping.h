#ifndef __SETTING_SLEEP_PAGE_H__
#define __SETTING_SLEEP_PAGE_H__

#include "page_manager.h"
#include "vm_setsleeping.h"
typedef struct 
{
    const char* name;
    page_vtable_t* page_param;
    setting_sleep_view_t* vw;
} setting_sleep_t;

page_vtable_t* setting_sleep_create(const char* name);

#endif
