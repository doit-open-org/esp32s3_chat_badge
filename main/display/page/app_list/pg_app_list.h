#ifndef APP_LIST_H
#define APP_LIST_H
#include "page_manager.h"
#include "vw_app_list.h"

typedef struct { 
    page_base_t base; 
    const char *name; 
    app_list_view_t *vw;
    page_vtable_t *page_param; 
} app_list_t;

page_vtable_t* app_list_create(const char *name);

#endif
