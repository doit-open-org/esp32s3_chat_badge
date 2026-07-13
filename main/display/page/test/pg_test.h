#ifndef __PG_TEST_H__
#define __PG_TEST_H__

#include "page_manager.h"
#include "vw_test.h"

typedef struct
{
    const char* name;
    page_vtable_t* page_param;
    test_view_t* view;
} test_pg_t;

page_vtable_t* test_create(const char* name);

#endif /* __PG_TEST_H__ */
