#include "lvgl.h"
#include "cl_ui.h"
#include "vw_startup.h"

// #include "lv_vpg/lv_vpg.h"

static startup_view_t vw;

#define TAG "vw_startup"

startup_view_t* startup_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);

    lv_obj_t *vpg = lv_img_create(root);
    lv_img_set_src(vpg, "P:/02.vpg");
    lv_obj_center(vpg);

    return &vw;
}

void startup_view_delete(void)
{
    /* 如需释放资源在此处理 */
}
