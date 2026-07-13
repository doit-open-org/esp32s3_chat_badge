#include "lvgl.h"
#include "pg_alarm_ring.h"
#include "vw_alarm_ring.h"
#include "cl_ui.h"
// #include "utlis/lcd_bl/lcd_bl_ctrl.h"
// #include <stdio.h>

static alarm_ring_t pg;


static void on_custom_attr_config(page_base_t *self)
{
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_FADE_ON, 300, lv_anim_path_ease_out);
}

/* Page load */
static void on_view_load(page_base_t *self)
{   
    // alarm_ring_model_init();
    pg.alarm_ring_view = alarm_ring_view_create(self->root);
}   

/* Page load complete */
static void on_view_did_load(page_base_t *self)
{
    // alarm_active_flag_set(1);
}

/* Page will be displayed soon  */
static void on_view_will_appear(page_base_t *self)
{

}

/* The page is displayed  */
static void on_view_did_appear(page_base_t *self)
{
    pg.alarm_ring_view->is_act = 1;
}

/* Page is about to disappear */
static void on_view_will_disappear(page_base_t *self)
{
    pg.alarm_ring_view->is_act = 0;
    // alarm_active_flag_set(0);
}

/* Page disappeared complete  */
static void on_view_did_disappear(page_base_t *self)
{
}

/* Page uninstall complete  */
static void on_view_did_unload(page_base_t *self)
{
    alarm_ring_view_delete();
}

static page_vtable_t page_param = {
    .on_custom_attr_config = on_custom_attr_config,
    .on_view_load = on_view_load,
    .on_view_did_load = on_view_did_load,
    .on_view_will_appear = on_view_will_appear,
    .on_view_did_appear = on_view_did_appear,
    .on_view_will_disappear = on_view_will_disappear,
    .on_view_did_disappear = on_view_did_disappear,
    .on_view_did_unload = on_view_did_unload,
};

page_vtable_t* alarm_ring_create(const char *name)
{
    pg.page_param = &page_param;
    pg.name = name;

    return pg.page_param;
}

REGIST_PAGE(alarm_ring)