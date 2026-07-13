#ifndef _PG_USB_MODE_H_
#define _PG_USB_MODE_H_

#include "page_manager/inc/page_manager.h"
#include "vm_usb_mode.h"

typedef struct {
    usb_mode_view_t *vw;
} usb_mode_t;

page_vtable_t* usb_mode_create(const char *name);

#endif
