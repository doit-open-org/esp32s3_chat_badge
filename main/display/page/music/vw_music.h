// #ifndef MUSIC_VIEW_H
// #define MUSIC_VIEW_H
// #include "lvgl.h"

// /* 拖拽上下/吸附所需的上下文 */
// typedef struct {
//     lv_obj_t *cont_bg;              // 被拖拽的对象
//     lv_obj_t *cont;              // 被拖拽的对象
//     lv_obj_t *trig_bar;
//     lv_obj_t *seg_btn;              // 被拖拽的对象
//     lv_coord_t press_y;        // 按下时指针 y
//     lv_coord_t obj_start_y;    // 按下时对象 y
//     lv_coord_t open_y;         // 完全显示时的 y（贴底展开）
//     lv_coord_t closed_y;       // 完全收起时的 y（向下滑出）
// } menu_drag_ctx_t;

// typedef struct
// {
//     bool is_act;
//     lv_obj_t *player_cont;
//     lv_obj_t *list_cont;

//     lv_obj_t *bg_img;

//     lv_obj_t *title_label;
//     lv_obj_t *lyrc_label;
//     lv_obj_t *time_curr;
//     lv_obj_t *time_total;
//     lv_obj_t *progress_bar;

//     lv_obj_t *play_btn;
//     lv_obj_t *priv_btn;
//     lv_obj_t *next_btn;
//     lv_obj_t *loop_btn;
//     lv_obj_t *list_btn;

//     lv_timer_t* get_info_timer;
//     menu_drag_ctx_t menu;
//     int total_time;
//     int curr_time;
//     int updata_time;
//     bool progress_lock;
// }music_view_t;

// void music_menu_next();
// void music_menu_priv();
// void music_menu_click();
// void music_get_list();
// music_view_t* music_view_create(lv_obj_t* root);
// void music_view_delete(void);
// void music_view_enable_drag(lv_obj_t *obj);
// #endif 