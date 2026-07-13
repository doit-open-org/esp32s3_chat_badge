#include "lvgl.h"
#include "pg_app_list.h"
#include "vw_app_list.h"
#include "esp_log.h"

static app_list_t pg;

static void app_list_event_cb(lv_event_t* event) {
    // 事件处理（如返回、定时跳转等）
}

static void on_custom_attr_config(page_base_t *self) {
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_NONE, 100, lv_anim_path_ease_out);
}

static void on_view_load(page_base_t *self) {
    pg.vw = app_list_view_create(self->root);
}

static void on_view_did_load(page_base_t *self) {
}

static void on_view_will_appear(page_base_t *self) {
}

static void on_view_did_appear(page_base_t *self) {
    pg.vw->is_act = 1;
}

static void on_view_will_disappear(page_base_t *self) {
    pg.vw->is_act = 0;
}

static void on_view_did_disappear(page_base_t *self) {
}

static void on_view_did_unload(page_base_t *self) {
    app_list_view_delete();
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

page_vtable_t* app_list_create(const char *name) {
    pg.page_param = &page_param;
    pg.name = name;
    return pg.page_param;
}

REGIST_PAGE(app_list)
