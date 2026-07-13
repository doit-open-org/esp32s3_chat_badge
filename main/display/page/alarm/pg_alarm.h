#ifndef __alarm_H__
#define __alarm_H__

#include "page_manager.h"
#include "vw_alarm.h"


typedef struct 
{
    const char* name;
    page_vtable_t* page_param;
    // alarm_model_t* alarm_model;
    alarm_view_t* alarm_view;
}alarm_t;

page_vtable_t* alarm_create(const char* name);

#endif