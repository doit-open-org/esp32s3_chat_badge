#ifndef VM_ANIMATION_H
#define VM_ANIMATION_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        uint8_t is_act;         /* 页面是否激活 */
        lv_obj_t *root;         /* 根对象 */
        lv_obj_t *bg_vpg;       /* VPG显示对象 */
        void *vpg_handle;       /* 图片列表句柄 */
        uint8_t ges_lock;       /* 手势锁定标志 */
        lv_obj_t *del_dialog;   /* 删除确认对话框 */
        lv_obj_t *delete_cont;  /* 删除进度容器 */
        lv_obj_t *delete_bar;   /* 删除进度条 */
        lv_obj_t *upload_img;   /* 上传提示图标 */
        lv_obj_t *upload_label; /* 上传提示文字 */
    } animation_view_t;

    void animation_view_delete(void);                                          /* 删除动画页面 */
    void animation_view_refresh_vpg(void);                                     /* 刷新VPG显示 */
    animation_view_t *animation_view_create(lv_obj_t *root, const char *path); /* 创建动画页面 */

#ifdef __cplusplus
}
#endif

#endif /* VM_ANIMATION_H */
