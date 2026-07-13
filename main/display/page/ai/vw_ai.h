#ifndef _VIEW_AI_H_
#define _VIEW_AI_H_

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

typedef struct
{
    uint32_t last_act;
    uint8_t is_act;
    lv_obj_t *speak_staus;
} vw_ai_t;

vw_ai_t *ai_view_create(lv_obj_t *root);
/**
 * @brief AI 页面销毁回调
 * @param root 页面根对象
 * @note 在页面销毁时清理背景资源
 */
void ai_view_destroy(lv_obj_t *root);

#endif