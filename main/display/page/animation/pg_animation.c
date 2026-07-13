
#define USE_VPG_DECODE

#include "pg_animation.h"
#include "doit_file.h"
#include "doit_ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "vpg_decode.h"
static animation_pg_t pg;
static void on_custom_attr_config(page_base_t *self)
{
    page_set_custom_cache_enable(self, false);
    page_set_custom_load_anim_type(self, LOAD_ANIM_NONE, 100,
                                   lv_anim_path_ease_out);
}

/* Page load */
static void on_view_load(page_base_t *self)
{
    pg.view = animation_view_create(self->root, NULL);
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
    doit_file_set_type(MATERIAL_TYPE_VPG);
    doit_file_set_refresh_display_callback(animation_view_refresh_vpg);
    if (doit_file_check_refresh_needed())
    {
        ESP_LOGI("pg_anim", "Refresh event detected, refreshing display");
    }

    animation_view_refresh_vpg();
}

/* Page is about to disappear */
static void on_view_will_disappear(page_base_t *self)
{
#ifdef USE_VPG_DECODE
    // VPG_Decode模式：停止播放器
    doit_vpg_player_stop();
#endif
    vTaskDelay(pdMS_TO_TICKS(50));
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
static void on_view_did_unload(page_base_t *self) { animation_view_delete(); }

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

page_vtable_t *animation_create(const char *name)
{
    pg.page_param = &page_param;
    pg.name = name;
    return pg.page_param;
}

REGIST_PAGE(animation)
