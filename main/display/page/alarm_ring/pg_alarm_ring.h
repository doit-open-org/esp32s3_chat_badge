#ifndef __alarm_ring_H__
#define __alarm_ring_H__

#include "page_manager.h"
#include "vw_alarm_ring.h"


typedef struct 
{
    const char* name;
    page_vtable_t* page_param;
    // alarm_ring_model_t* alarm_ring_model;
    alarm_ring_view_t* alarm_ring_view;
}alarm_ring_t;

page_vtable_t* alarm_ring_create(const char* name);

#endif