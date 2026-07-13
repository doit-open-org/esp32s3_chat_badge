#ifndef HELP_H
#define HELP_H

#include "page_manager.h"
#include "vm_help.h"
typedef struct {
    page_base_t base;
    void *vw;
} help_t;

page_vtable_t* help_create(const char *name);

#endif
