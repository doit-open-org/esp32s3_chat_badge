#ifndef _PAGE_AI_H_
#define _PAGE_AI_H_

#include "page_manager.h"
#include "vw_ai.h"

typedef struct
{
    const char *name;
    page_vtable_t *page_param;
    vw_ai_t *vw;
} pg_ai_t;

page_vtable_t *ai_create(const char *name);

#endif