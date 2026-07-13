#ifndef __PG_STARTUP_H__
#define __PG_STARTUP_H__

#include "page_manager.h"
#include "vw_startup.h"

typedef struct
{
    const char* name;
    page_vtable_t* page_param;
    startup_view_t* view;
} startup_pg_t;

page_vtable_t* startup_create(const char* name);

#endif /* __PG_STARTUP_H__ */
