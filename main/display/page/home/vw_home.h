#ifndef __HOME_VIEW_H__
#define __HOME_VIEW_H__

#include "lvgl.h"

typedef struct
{
    uint8_t is_act;        /* 页面是否激活 */
    lv_obj_t *root;        /* 根对象 */
    lv_obj_t *bg_img;      /* 背景图片对象 */
    void *jpg_handle;      /* 图片列表句柄 */
    uint8_t ges_lock;      /* 手势锁定标志 */
    lv_obj_t *del_dialog;  /* 删除确认对话框 */
    lv_obj_t *delete_cont; /* 删除进度容器 */
    lv_obj_t *delete_bar;  /* 删除进度条 */
    lv_obj_t *vpg_obj;
    lv_obj_t *upload_img;   /* 上传提示图标 */
    lv_obj_t *upload_label; /* 上传提示文字 */
} home_view_t;

// home_view_t* home_view_create(lv_obj_t *root);
home_view_t *home_view_create(lv_obj_t *root, const char *path);
void home_view_delete(void);
void home_view_refresh_image(void); /* 刷新当前显示的图片 */

#endif /* __HOME_VIEW_H__ */
