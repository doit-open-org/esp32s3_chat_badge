#include "lvgl.h"
#include "pg_about.h"
static about_t pg;

static void on_custom_attr_config(page_base_t *self) {
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_NONE, 100, lv_anim_path_ease_out);
}

static void on_view_load(page_base_t *self) {
    pg.vw = about_view_create(self->root);
}

static void on_view_did_load(page_base_t *self) {
}

static void on_view_will_appear(page_base_t *self) {
}

static void on_view_did_appear(page_base_t *self) {
    if (pg.vw != NULL) {
        ((about_view_t*)pg.vw)->is_act = 1;
    }
}

static void on_view_will_disappear(page_base_t *self) {
    if (pg.vw != NULL) {
        ((about_view_t*)pg.vw)->is_act = 0;
    }
}

static void on_view_did_disappear(page_base_t *self) {
}

static void on_view_did_unload(page_base_t *self) {
    about_view_delete();
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

page_vtable_t* about_create(const char *name) {
    return &page_param;
}

REGIST_PAGE(about)
