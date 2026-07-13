#ifndef ABOUT_H
#define ABOUT_H
#include "vm_about.h"
#include "page_manager.h"

typedef struct {
    page_base_t base;
    void *vw;
} about_t;

page_vtable_t* about_create(const char *name);

#endif
