#include "lvgl.h"
#include "pg_wifi_config.h"
#include "vm_wifi_config.h"

static wifi_config_t pg;

static void on_custom_attr_config(page_base_t *self)
{
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_FADE_ON, 300, lv_anim_path_ease_out);
}

static void on_view_load(page_base_t *self)
{
    pg.vw = wifi_config_view_create(self->root);
}

static void on_view_did_load(page_base_t *self)
{
}

static void on_view_will_appear(page_base_t *self)
{
}

static void on_view_did_appear(page_base_t *self)
{
    if (pg.vw != NULL)
    {
        wifi_config_view_set_active(true);
    }
}

static void on_view_will_disappear(page_base_t *self)
{
    if (pg.vw != NULL)
    {
        wifi_config_view_set_active(false);
    }
}

static void on_view_did_disappear(page_base_t *self)
{
}

static void on_view_did_unload(page_base_t *self)
{
    wifi_config_view_delete();
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

page_vtable_t *wifi_config_create(const char *name)
{
    pg.page_param = &page_param;
    pg.name = name;
    return &page_param;
}

REGIST_PAGE(wifi_config)
