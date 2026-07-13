#ifndef _VM_USB_MODE_H_
#define _VM_USB_MODE_H_

#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *status_label;
    int is_act;
} usb_mode_view_t;

usb_mode_view_t* usb_mode_view_create(lv_obj_t *root);
void usb_mode_view_delete(void);

#endif
