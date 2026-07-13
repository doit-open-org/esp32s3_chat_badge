/**
 * @file lv_vpg.h
 *
 */

#ifndef LV_VPG_H
#define LV_VPG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
// #include "lv_conf_internal.h"
// #include "misc/lv_types.h"
// #include "draw/lv_draw_buf.h"
// #include "widgets/image/lv_image.h"
// #include "core/lv_obj_class.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct lv_vpg_t lv_vpg_t;
LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t lv_vpg_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a gif object
 * @param parent    pointer to an object, it will be the parent of the new gif.
 * @return          pointer to the gif obj
 */
lv_obj_t * lv_vpg_create(lv_obj_t * parent);

/**
 * Set the vpg data to display on the object
 * @param obj       pointer to a vpg object
 * @param src       1) pointer to an ::lv_image_dsc_t descriptor (which contains gif raw data) or
 *                  2) path to a vpg file (e.g. "S:/dir/anim.vpg")
 */
void lv_vpg_set_src(lv_obj_t * obj, const void * src);


/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_GIF_H*/
