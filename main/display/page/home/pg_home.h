#ifndef __PG_HOME_H__
#define __PG_HOME_H__

#include "page_manager.h"
#include "vw_home.h"

typedef struct
{
    const char* name;
    page_vtable_t* page_param;
    home_view_t* view;
} home_pg_t;

page_vtable_t* home_create(const char* name);

#endif /* __PG_HOME_H__ */
