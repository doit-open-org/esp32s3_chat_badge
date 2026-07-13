#ifndef __VW_APP_LIST_H__
#define __VW_APP_LIST_H__

#include "lvgl.h"

typedef struct { 
    uint8_t is_act;         /* 页面是否激活 */
    lv_obj_t *root;         /* 根对象 */
} app_list_view_t;

app_list_view_t* app_list_view_create(lv_obj_t *root);
void app_list_view_delete(void);

#endif /* __VW_APP_LIST_H__ */
