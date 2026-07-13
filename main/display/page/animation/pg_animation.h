#ifndef PG_ANIMATION_H
#define PG_ANIMATION_H

#include "page_manager.h"
#include "vm_animation.h"

typedef struct
{
    const char *name;
    page_vtable_t *page_param;
    animation_view_t *view;
} animation_pg_t;

page_vtable_t* animation_create(const char *name);

#endif
