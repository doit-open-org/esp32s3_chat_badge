#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H
#include "page_manager.h"
#include "vm_wifi_config.h"
typedef struct { 
    page_base_t base;
    const char *name; 
    wifi_config_view_t *vw;
    page_vtable_t *page_param; 
} wifi_config_t;

page_vtable_t* wifi_config_create(const char *name);

#endif
