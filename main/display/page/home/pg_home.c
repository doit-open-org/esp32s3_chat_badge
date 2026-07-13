#include "pg_home.h"
#include "cl_ui.h"
#include "doit_file.h"
#include "lvgl.h"
#include "vw_home.h"

static home_pg_t pg;

static void on_custom_attr_config(page_base_t *self)
{
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_NONE, 100,
                                   lv_anim_path_ease_out);
    // page_set_custom_load_anim_type(self, LOAD_ANIM_FADE_ON, 250,
    //                                lv_anim_path_ease_in);
}

/* Page load */
static void on_view_load(page_base_t *self)
{
    pg.view = home_view_create(self->root, NULL);
}

/* Page load complete */
static void on_view_did_load(page_base_t *self) {}

/* Page will be displayed soon  */
static void on_view_will_appear(page_base_t *self)
{
    lv_obj_clear_flag(self->root, LV_OBJ_FLAG_HIDDEN);
}

/* The page is displayed  */
static void on_view_did_appear(page_base_t *self)
{
    pg.view->is_act = 1;
    doit_file_set_type(MATERIAL_TYPE_IMG);
    doit_file_set_refresh_display_callback(home_view_refresh_image);
    ESP_LOGI("pg_home", "appear p=%s act=%d", get_show_dir(), pg.view->is_act);

    if (doit_file_check_refresh_needed())
    {
        ESP_LOGI("pg_home", "Refresh event detected, refreshing display");
    }

    home_view_refresh_image();
}

/* Page is about to disappear */
static void on_view_will_disappear(page_base_t *self)
{
    pg.view->is_act = 0;
    doit_file_set_refresh_display_callback(NULL);
#ifndef SIMULATOR
#endif
}

/* Page disappeared complete  */
static void on_view_did_disappear(page_base_t *self)
{
    lv_obj_add_flag(self->root, LV_OBJ_FLAG_HIDDEN);
}

/* Page uninstall complete  */
static void on_view_did_unload(page_base_t *self) { home_view_delete(); }

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

page_vtable_t *home_create(const char *name)
{
    pg.page_param = &page_param;
    pg.name = name;
    return pg.page_param;
}

REGIST_PAGE(home)
