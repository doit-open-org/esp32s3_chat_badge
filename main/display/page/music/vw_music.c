// #include "vw_music.h"
// #include "cl_ui.h"
// #include <string.h>
// #ifndef SIMULATOR
// #include "vb6824.h"
// // #include "vb_api.h"
// #include "esp_lvgl_port.h"
// #endif

// LV_FONT_DECLARE(font_puhui_20_4)

// LV_IMG_DECLARE(img_vinyl_red);       
// LV_IMG_DECLARE(icon_music_one);          
// LV_IMG_DECLARE(icon_priv_32);
// LV_IMG_DECLARE(icon_pause_32);
// LV_IMG_DECLARE(icon_play_32);
// LV_IMG_DECLARE(icon_next_32);

// LV_IMG_DECLARE(icon_loop_32);
// LV_IMG_DECLARE(icon_loop1_32);
// LV_IMG_DECLARE(icon_rand_32);
// LV_IMG_DECLARE(icon_music_list_24);

// LV_IMG_DECLARE(icon_tf_16);
// LV_IMG_DECLARE(icon_bt_16);
// LV_IMG_DECLARE(icon_bright_16);
// LV_IMG_DECLARE(icon_vol_16);

// LV_IMG_DECLARE(icon_music_cy);
// LV_IMG_DECLARE(icon_music_rand);
// LV_IMG_DECLARE(icon_music_bt);
// LV_IMG_DECLARE(icon_music_sd);

// #define TAG "music_view"

// static music_view_t vw = {0};

// #define MUSIC_PAGE_SIZE    10
// #define MUSIC_WINDOW_MAX  20

// typedef void (*music_data_request_cb_t)(uint32_t start,
//                                         uint32_t count,
//                                         void *user_data);

// static lv_obj_t *g_music_list = NULL;

// static uint32_t g_loaded_start = 0;
// static uint32_t g_loaded_end   = 0;
// static uint32_t g_total_count  = 0;

// static music_data_request_cb_t g_req_cb = NULL;
// static void *g_req_user_data = NULL;

// typedef enum {
//     REQ_NONE = 0,
//     REQ_BOTTOM,
//     REQ_TOP,
// } req_dir_t;

// static uint32_t g_req_tick = 0;
// static uint32_t g_play_index = 0xffffffff;
// static bool g_req_in_progress = false;
// static req_dir_t g_req_dir = REQ_NONE;
// static uint32_t g_req_start = 0;
// static uint32_t g_req_count = 0;

// // /* 防跳锚点 */
// static uint32_t g_anchor_index = UINT32_MAX;


// static void song_click_event_cb(lv_event_t *e){
// #ifndef SIMULATOR
//     uint32_t index = (uint32_t)(intptr_t)lv_event_get_user_data(e);
//     // vb_music_play_by_index(index);
// #endif
// }


// // /* ---------------- 创建 item ---------------- */
// static lv_obj_t *create_item(uint32_t index, const char *name)
// {
//     lv_obj_t *item = lv_obj_create(g_music_list);
//     lv_obj_set_user_data(item, (void *)(uintptr_t)index);
//     lv_obj_set_size(item, 260, 45);
//     lv_obj_set_style_bg_color(item, lv_color_hex(0x2b2b2b), 0);
//     lv_obj_set_style_bg_color(item, lv_color_hex(0xff5353), LV_PART_MAIN | LV_STATE_CHECKED);
//     lv_obj_set_style_border_width(item, 0, 0);
//     lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_add_event_cb(item, song_click_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)index);

//     if (index == g_play_index) {
//         lv_obj_add_state(item, LV_STATE_CHECKED);
//     }

//     lv_obj_t* text = lv_label_create(item);
//     lv_label_set_long_mode(text, LV_LABEL_LONG_SCROLL_CIRCULAR);
//     // lv_label_set_long_mode(text, LV_LABEL_LONG_DOT);
//     lv_obj_set_width(text, 260); 
//     lv_obj_align(text, LV_ALIGN_LEFT_MID, 5, 0);
//     lv_obj_set_style_text_font(text, &font_puhui_20_4, 0);
//     lv_obj_set_scrollbar_mode(text, LV_SCROLLBAR_MODE_AUTO);
//     lv_label_set_text(text, name);
//     lv_obj_set_style_text_color(text, lv_color_white(), 0);

//     return item;
// }

// // /* ---------------- 锚点回对齐 ---------------- */
// static void scroll_to_anchor(void)
// {
//     if (g_anchor_index == UINT32_MAX) return;

//     uint32_t cnt = lv_obj_get_child_count(g_music_list);
//     for (uint32_t i = 0; i < cnt; i++) {
//         lv_obj_t *c = lv_obj_get_child(g_music_list, i);
//         uint32_t idx = (uint32_t)(uintptr_t)lv_obj_get_user_data(c);
//         if (idx == g_anchor_index) {
//             lv_obj_scroll_to_view(c, LV_ANIM_OFF);
//             break;
//         }
//     }
//     g_anchor_index = UINT32_MAX;
// }

// // /* ---------------- 第一次加载请求指定页 ---------------- */
// static void request_page(uint32_t page_index)
// {
//     uint32_t tick =  lv_tick_get();
//     if (tick - g_req_tick > 1000 && g_req_in_progress){
//         g_req_in_progress = false;
//     }
   
//     if (!g_req_cb || g_req_in_progress) return;
//     if (g_total_count > 0 && g_loaded_end >= g_total_count) return;

//     g_anchor_index = page_index;
    

//     uint32_t start = page_index/MUSIC_PAGE_SIZE*MUSIC_PAGE_SIZE;
//     uint32_t cnt = MUSIC_PAGE_SIZE;
//     if (g_total_count > 0 && start + cnt > g_total_count)
//         cnt = g_total_count - start;

//     g_loaded_start = start;
//     g_loaded_end = start;
//     g_req_in_progress = true;
//     g_req_dir = REQ_BOTTOM;
//     g_req_start = start;
//     g_req_count = cnt;
//     ESP_LOGI(TAG, "request_page: start=%lu, cnt=%lu", start, cnt);
//     g_req_cb(start, cnt, g_req_user_data);
// }

// // /* ---------------- 请求下一页 ---------------- */
// static void request_down(void)
// {
//     uint32_t tick =  lv_tick_get();
//     if (tick - g_req_tick > 1000 && g_req_in_progress){
//         g_req_in_progress = false;
//     }
   
//     if (!g_req_cb || g_req_in_progress) return;
//     if (g_total_count > 0 && g_loaded_end >= g_total_count) return;

//     /* ✔ 锚点：倒数第二行 */
//     if (g_loaded_end > g_loaded_start) {
//         uint32_t win_cnt = g_loaded_end - g_loaded_start;
//         uint32_t anchor_pos = (win_cnt >= 2) ? win_cnt - 2 : win_cnt - 1;
//         g_anchor_index = g_loaded_start + anchor_pos;
//     }

//     uint32_t start = g_loaded_end;
//     uint32_t cnt = MUSIC_PAGE_SIZE;
//     if (g_total_count > 0 && start + cnt > g_total_count)
//         cnt = g_total_count - start;

//     g_req_in_progress = true;
//     g_req_dir = REQ_BOTTOM;
//     g_req_start = start;
//     g_req_count = cnt;

//     g_req_cb(start, cnt, g_req_user_data);
// }
// // /* ---------------- 请求上一页 ---------------- */
// static void request_up(void)
// {
//     if (!g_req_cb || g_req_in_progress) return;
//     if (g_loaded_start == 0) return;

//     /* ✔锚点：第二行（若有），避免看到标题 */
//     if (g_loaded_end > g_loaded_start) {
//         uint32_t win_cnt = g_loaded_end - g_loaded_start;
//         uint32_t anchor_pos = (win_cnt >= 2) ? 1 : 0;
//         g_anchor_index = g_loaded_start + anchor_pos;
//     }

//     uint32_t cnt = MUSIC_PAGE_SIZE;
//     if (g_loaded_start < cnt) cnt = g_loaded_start;

//     uint32_t start = g_loaded_start - cnt;

//     g_req_in_progress = true;
//     g_req_dir = REQ_TOP;
//     g_req_start = start;
//     g_req_count = cnt;

//     g_req_cb(start, cnt, g_req_user_data);
// }

// // /* ---------------- 下拉后的追加逻辑 ---------------- */
// static void feed_down(uint32_t start, uint32_t count, const char *names[])
// {
//     ESP_LOGI(TAG, "feed_down: start=%lu, count=%lu", start, count);
//     if (start != g_loaded_end) return;

//     for (uint32_t i = 0; i < count; i++)
//         create_item(start + i, names[i]);

//     g_loaded_end += count;

//     /* 超过 10 条 → 删掉顶部 */
//     while ((g_loaded_end - g_loaded_start) > MUSIC_WINDOW_MAX) {
//         lv_obj_t *first = lv_obj_get_child(g_music_list, 0);
//         lv_obj_del(first);
//         g_loaded_start++;
//     }

//     scroll_to_anchor();
// }

// // /* ---------------- 上拉后的补回逻辑 ---------------- */
// static void feed_up(uint32_t start, uint32_t count, const char *names[])
// {
//     if (start + count != g_loaded_start) return;

//     /* 插入顶部 */
//     for (uint32_t i = 0; i < count; i++) {
//         lv_obj_t *btn = create_item(start + i, names[i]);
//         lv_obj_move_to_index(btn, i);
//     }

//     g_loaded_start = start;

//     /* 超过 10 条 → 删掉底部 */
//     while ((g_loaded_end - g_loaded_start) > MUSIC_WINDOW_MAX) {
//         uint32_t c = lv_obj_get_child_count(g_music_list);
//         lv_obj_t *last = lv_obj_get_child(g_music_list, c - 1);
//         lv_obj_del(last);
//         g_loaded_end--;
//     }

//     scroll_to_anchor();
// }

// // /* ---------------- 外部喂数据接口 ---------------- */
// void music_list_feed(uint32_t start,
//                      uint32_t count,
//                      const char *names[],
//                      uint32_t total)
// {
//     if (!g_req_in_progress || start != g_req_start)
//         return;

//     if (total > 0) g_total_count = total;
//     if (g_req_dir == REQ_BOTTOM) 
//         feed_down(start, count, names);
//     else       
//         feed_up(start, count, names);

//     g_req_in_progress = false;
//     g_req_dir = REQ_NONE;
// }

// // /* ---------------- 滚动事件：决定上下翻页 ---------------- */
// static void scroll_cb(lv_event_t *e)
// {
//     lv_obj_t *list = lv_event_get_target(e);
//     // ESP_LOGI(TAG, "scroll_cb: top %d bottom %d", (int)lv_obj_get_scroll_top(list), (int)lv_obj_get_scroll_bottom(list));
//     if (lv_obj_get_scroll_bottom(list) < 10)
//         request_down();

//     if (lv_obj_get_scroll_top(list) < 10)
//         request_up();
// }

// // /* ---------------- 创建控件 ---------------- */
// lv_obj_t *music_list_create(lv_obj_t *parent,
//                              music_data_request_cb_t req_cb,
//                              void *user_data)
// {
//     g_req_cb = req_cb;
//     g_req_user_data = user_data;

//     lv_obj_t *cont = lv_obj_create(parent);
//     lv_obj_set_style_border_width(cont, 0, 0);
//     lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_pad_all(cont, 0, 0);
//     lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
//     lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

//     lv_obj_t* status_bar_label = lv_label_create(cont);
//     lv_label_set_text(status_bar_label, LT_MUSIC_LIST);
//     lv_obj_set_style_text_font(status_bar_label, &font_puhui_20_4, 0);
//     lv_obj_set_style_text_color(status_bar_label, lv_color_hex(0xFFFFFF), 0);
//     lv_obj_align(status_bar_label, LV_ALIGN_TOP_MID, 0, 30);

//     /* 列表容器（替换原 LVGL8 lv_list_create） */
//     g_music_list = lv_obj_create(cont);
//     lv_obj_set_size(g_music_list, LV_PCT(75), LV_PCT(70));
//     lv_obj_set_style_bg_opa(g_music_list, LV_OPA_TRANSP, 0);
//     lv_obj_align(g_music_list, LV_ALIGN_TOP_MID, 10, 70);
//     lv_obj_set_style_border_width(g_music_list, 0, 0);
//     lv_obj_set_style_radius(g_music_list, 0, 0);
//     lv_obj_set_style_pad_row(g_music_list, 0, 20);
//     lv_obj_set_style_pad_all(g_music_list, 0, 0);
//     lv_obj_set_style_bg_color(g_music_list, lv_color_hex(0x000000), 0);

//     lv_obj_set_scroll_dir(g_music_list, LV_DIR_VER);
//     lv_obj_set_flex_flow(g_music_list, LV_FLEX_FLOW_COLUMN);

//     lv_obj_add_event_cb(g_music_list, scroll_cb, LV_EVENT_SCROLL, NULL);

//     g_loaded_start = 0;
//     g_loaded_end = 0;

//     /* 请求第一页 */
//     // request_down();

//     return cont;
// }

// /********************************
//  * 上拉控制中心
//  *******************************/
// static void _menu_anim_close_cb(lv_anim_t *a){
//     lv_obj_set_width(vw.menu.trig_bar, 35);
//     lv_obj_add_flag(vw.menu.cont_bg, LV_OBJ_FLAG_HIDDEN);
//     lv_obj_set_style_bg_opa(vw.menu.cont, LV_OPA_0, 0);
// }

// // /* 统一的 y 动画 */
// static void menu_anim_to_y(lv_obj_t *obj, lv_coord_t to_y, uint32_t time_ms, lv_anim_completed_cb_t cb)
// {
//     lv_anim_t a;
//     lv_anim_init(&a);
//     lv_anim_set_var(&a, obj);
//     lv_anim_set_values(&a, lv_obj_get_y(obj), to_y);
//     lv_anim_set_time(&a, time_ms);
//     lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
//     lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
//     if (cb)
//     {
//         lv_anim_set_completed_cb(&a, cb);
//     }
//     lv_anim_start(&a);
// }


// static void menu_open(lv_obj_t *obj, menu_drag_ctx_t *ctx)  { menu_anim_to_y(obj, ctx->open_y,   100, NULL); }
// static void menu_close(lv_obj_t *obj, menu_drag_ctx_t *ctx) { menu_anim_to_y(obj, ctx->closed_y, 100, _menu_anim_close_cb); }

// /* 事件回调：实现只允许竖直拖拽 + 吸附 */
// static void menu_drag_event_cb(lv_event_t *e)
// {
//     menu_drag_ctx_t *ctx = (menu_drag_ctx_t *)lv_event_get_user_data(e);
//     lv_obj_t *obj = ctx->cont;
//     lv_event_code_t code = lv_event_get_code(e);

//     if(code == LV_EVENT_PRESSED) {
//         lv_obj_set_width(ctx->trig_bar, 48);   // 按下时，扩大拖拽手柄的宽度，方便拖拽
//         lv_obj_clear_flag(ctx->cont_bg, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_set_style_bg_opa(ctx->cont, LV_OPA_COVER, 0);
//         lv_point_t p; lv_indev_t *indev = lv_indev_get_act();
//         lv_indev_get_point(indev, &p);
//         ctx->press_y     = p.y;
//         ctx->obj_start_y = lv_obj_get_y(obj);
//     }
//     else if(code == LV_EVENT_PRESSING) {
//         lv_point_t p; lv_indev_t *indev = lv_indev_get_act();
//         lv_indev_get_point(indev, &p);

//         lv_coord_t dy    = p.y - ctx->press_y;          // 只看 Y，忽略 X -> 仅上下拖
//         lv_coord_t new_y = ctx->obj_start_y + dy;
        
//         /* 夹在 open_y 与 closed_y 之间，避免越界 */
//         if(new_y < ctx->open_y)   new_y = ctx->open_y;
//         if(new_y > ctx->closed_y) new_y = ctx->closed_y;

//         lv_obj_set_y(obj, new_y);
//     }
//     else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
//         lv_point_t p; lv_indev_t *indev = lv_indev_get_act();
//         lv_indev_get_point(indev, &p);
//         lv_coord_t diff = p.y - ctx->press_y;;

//         if (ctx->obj_start_y == 180 )
//         {
//             if (diff >= 10)
//             {
//                 menu_close(obj, ctx);             // 向下拖超过 10px -> 吸附收起
//                 return;
//             }
//             else
//             {
//                 menu_open(obj, ctx);              // 否则 -> 吸附展开
//                 return;
//             }
            
//         }else{
//             if (diff <= -20)
//             {
//                 menu_open(obj, ctx);              // 向上拖超过 10px -> 吸附展开
//                 return;
//             }
//             else
//             {
//                 menu_close(obj, ctx);             // 向下拖超过 10px -> 吸附收起
//                 return;
//             }
            
//         }
//     }
// }
// static void _on_bg_mask_click(lv_event_t *e){
//     if (vw.menu.cont == NULL) {
//         return;
//     }
//     lv_event_code_t code = lv_event_get_code(e);
//     if (code == LV_EVENT_RELEASED) {
//         menu_close(vw.menu.cont, &vw.menu);
//     }
// }


// lv_obj_t *music_player_create(lv_obj_t *root){

//     lv_obj_t *player_cont = lv_obj_create(root);
//     lv_obj_set_style_border_width(player_cont, 0, 0);
//     lv_obj_set_style_bg_color(player_cont, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_pad_all(player_cont, 0, 0);
//     lv_obj_set_size(player_cont, LV_HOR_RES, LV_VER_RES);
//     lv_obj_clear_flag(player_cont, LV_OBJ_FLAG_SCROLLABLE);
   
//     lv_obj_t *title = lv_label_create(player_cont);
//     vw.title_label = title;
//     lv_obj_set_width(title, 120);
//     lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
//     lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
//     lv_obj_set_style_text_font(title, &font_puhui_20_4, 0);
//     lv_label_set_text(title, LT_MUSIC_TITLE);
//     lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
//     lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

//     lv_obj_t *img_vinyl = lv_img_create(player_cont);
//     vw.bg_img = img_vinyl;
//     lv_obj_set_style_border_width(img_vinyl, 0, 0);
//     lv_img_set_src(img_vinyl, &img_vinyl_red);
//     lv_obj_align(img_vinyl, LV_ALIGN_CENTER, 0, -10);
//     lv_obj_move_background(img_vinyl);

//     lv_obj_t *lyrc = lv_label_create(player_cont);
//     vw.lyrc_label = lyrc;
//     lv_obj_set_width(lyrc, 200);
//     lv_obj_set_style_text_align(lyrc, LV_TEXT_ALIGN_CENTER, 0);
//     lv_label_set_long_mode(lyrc, LV_LABEL_LONG_SCROLL_CIRCULAR);
//     lv_obj_set_style_text_color(lyrc, lv_color_hex(0xFFFFFF), 0);
//     lv_obj_set_style_text_font(lyrc, &font_puhui_20_4, 0);
//     lv_label_set_text(lyrc, " ");
//     lv_obj_align_to(lyrc, img_vinyl, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
//     lv_obj_clear_flag(lyrc, LV_OBJ_FLAG_PRESS_LOCK);
    
//     lv_obj_t* progress_bar = lv_slider_create(player_cont);
//     vw.progress_bar = progress_bar;
//     lv_obj_set_size(progress_bar, 200,32); 
//     lv_obj_align_to(progress_bar, lyrc, LV_ALIGN_OUT_BOTTOM_MID, 0, -6);
//     lv_slider_set_range(progress_bar, 0, 100);
//     lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xCCCCCC), LV_PART_MAIN);         // 背景色
//     lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);     // 已滑动区域
//     lv_obj_set_style_border_width(progress_bar, 14, LV_PART_MAIN);
//     lv_obj_set_style_border_side(progress_bar, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_TOP, 0);
//     lv_obj_set_style_pad_top(progress_bar, 19, LV_PART_MAIN);
//     lv_obj_set_style_pad_bottom(progress_bar, 19, LV_PART_MAIN);
//     lv_obj_set_style_opa(progress_bar, LV_OPA_0, LV_PART_KNOB);
//     lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_GESTURE_BUBBLE); 
//     lv_obj_move_background(progress_bar);
//     int percent = 0;
//     if (vw.total_time != 0)
//     {
//         percent = vw.curr_time*100/vw.total_time;
//     }
//     lv_slider_set_value(progress_bar, percent, LV_ANIM_OFF);
    

//     lv_obj_t *time_curr = lv_label_create(player_cont);
//     vw.time_curr = time_curr;
//     lv_obj_set_style_text_color(time_curr, lv_color_hex(0xFFFFFF), 0);
//     lv_label_set_text_fmt(time_curr, "%02d:%02d", vw.curr_time/1000/60, (vw.curr_time/1000) % 60);
//     lv_obj_align_to(time_curr, progress_bar, LV_ALIGN_OUT_BOTTOM_LEFT, 6, -8);
    
//     lv_obj_t *time_total = lv_label_create(player_cont);
//     vw.time_total = time_total;
//     lv_obj_set_style_text_color(time_total, lv_color_hex(0xFFFFFF), 0);
//     lv_label_set_text_fmt(time_total, "%02d:%02d", vw.total_time/1000/60, (vw.total_time/1000) % 60);
//     lv_obj_align_to(time_total, progress_bar, LV_ALIGN_OUT_BOTTOM_RIGHT, -6, -8);


//     lv_obj_t *btn_play = lv_btn_create(player_cont);
//     vw.play_btn = btn_play;
//     lv_obj_set_size(btn_play, 50, 50);
//     lv_obj_set_style_radius(btn_play, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_bg_opa(btn_play, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_shadow_width(btn_play, 0, 0);
//     lv_obj_align_to(btn_play, img_vinyl, LV_ALIGN_CENTER, 0, 0);
//     lv_obj_t *btn_img_play = lv_img_create(btn_play);
// #ifndef SIMULATOR
//     // if (vb_music_get_play_status()==1)
//     // {
//     //     lv_img_set_src(btn_img_play, &icon_pause_32);
//     // }else{
//     //     lv_img_set_src(btn_img_play, &icon_play_32);
//     // }
// #else
//     lv_img_set_src(btn_img_play, &icon_play_32);
// #endif 
//     lv_obj_center(btn_img_play);

//     lv_obj_t *btn_prev = lv_btn_create(player_cont);
//     vw.priv_btn = btn_prev;
//     lv_obj_set_size(btn_prev, 50, 50);
//     lv_obj_set_style_radius(btn_prev, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_bg_opa(btn_prev, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_shadow_width(btn_prev, 0, 0);
//     lv_obj_align_to(btn_prev, btn_play, LV_ALIGN_OUT_LEFT_MID, -30, 0);
//     lv_obj_t *btn_img_prev = lv_img_create(btn_prev);
//     lv_img_set_src(btn_img_prev, &icon_priv_32);
//     lv_obj_center(btn_img_prev);

//     lv_obj_t *btn_next = lv_btn_create(player_cont);
//     vw.next_btn = btn_next;
//     lv_obj_set_size(btn_next, 50, 50);
//     lv_obj_set_style_radius(btn_next, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_bg_opa(btn_next, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_shadow_width(btn_next, 0, 0);
//     lv_obj_align_to(btn_next, btn_play, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
//     lv_obj_t *btn_img_next = lv_img_create(btn_next);
//     lv_img_set_src(btn_img_next, &icon_next_32);
//     lv_obj_center(btn_img_next);

//     lv_obj_t *btn_loop = lv_btn_create(player_cont);
//     vw.loop_btn = btn_loop;
//     lv_obj_set_size(btn_loop, 50, 50);
//     lv_obj_set_style_radius(btn_loop, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_bg_opa(btn_loop, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_shadow_width(btn_loop, 0, 0);
//     lv_obj_align_to(btn_loop, btn_play, LV_ALIGN_OUT_TOP_MID, 0, -30);
//     lv_obj_t *btn_img_loop = lv_img_create(btn_loop);
//     lv_img_set_src(btn_img_loop, &icon_loop_32);
//     lv_obj_center(btn_img_loop);

//     lv_obj_t *btn_list = lv_btn_create(player_cont);
//     vw.list_btn = btn_list;
//     lv_obj_set_size(btn_list, 50, 50);
//     lv_obj_set_style_radius(btn_list, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_bg_opa(btn_list, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_shadow_width(btn_list, 0, 0);
//     lv_obj_align_to(btn_list, btn_play, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
//     lv_obj_t *btn_img_list = lv_img_create(btn_list);
//     lv_img_set_src(btn_img_list, &icon_music_list_24);
//     lv_obj_center(btn_img_next);
// #ifndef SIMULATOR
//     // vb_music_mode_t mode = vb_music_get_mode();
//     // if (mode != VB_MUSIC_MODE_TF)
//     // {
//     //     lv_obj_add_flag(btn_list, LV_OBJ_FLAG_HIDDEN);
//     //     lv_obj_add_flag(btn_loop, LV_OBJ_FLAG_HIDDEN);
//     // }
// #endif
//     return player_cont;
// }

// static void _on_bt_btn_click(lv_event_t *e){
//     lv_event_code_t code = lv_event_get_code(e);
//     if (code == LV_EVENT_CLICKED) { 
// #ifndef SIMULATOR
//         // vb_music_set_mode(0);
// #endif 
//     }
// }

// static void _on_tf_btn_click(lv_event_t *e){
//     lv_event_code_t code = lv_event_get_code(e);
//     if (code == LV_EVENT_CLICKED) { 
// #ifndef SIMULATOR
//         // vb_music_set_mode(1);
// #endif
//     }
// }

// static void _on_mode_change(lv_event_t *e){
//     uint32_t *status = lv_event_get_param(e);
//     lv_obj_t *btn_bt = lv_obj_get_child(vw.menu.seg_btn, 0);
//     lv_obj_t *btn_tf = lv_obj_get_child(vw.menu.seg_btn, 1);
// #ifndef SIMULATOR
//     // if ((vb_music_mode_t)status[0] == VB_MUSIC_MODE_BT)
//     // {
//     //     lv_obj_add_flag(vw.list_btn, LV_OBJ_FLAG_HIDDEN);
//     //     lv_obj_add_flag(vw.loop_btn, LV_OBJ_FLAG_HIDDEN);
//     //     lv_obj_remove_flag(vw.progress_bar, LV_OBJ_FLAG_CLICKABLE);
//     //     lv_obj_add_state(btn_bt, LV_STATE_CHECKED);
//     //     lv_obj_remove_state(btn_tf, LV_STATE_CHECKED);
//     // }
//     // else{
//         lv_obj_remove_flag(vw.list_btn, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_remove_flag(vw.loop_btn, LV_OBJ_FLAG_HIDDEN);
//         lv_obj_add_flag(vw.progress_bar, LV_OBJ_FLAG_CLICKABLE);
//         lv_obj_add_state(btn_tf, LV_STATE_CHECKED);
//         lv_obj_remove_state(btn_bt, LV_STATE_CHECKED);
//     // }
// #endif
// }

// static void _on_play_status(lv_event_t *e){
//     lv_obj_t *btn_img = lv_obj_get_child(vw.play_btn, 0);
//     if (!btn_img)
//     {
//         return;
//     }
//     uint32_t *status = lv_event_get_param(e);
//     if (status[0] == 0)
//     {
//         lv_img_set_src(btn_img, &icon_play_32);
//     }else{
//         lv_img_set_src(btn_img, &icon_pause_32);
//     }
// }

// static void _on_music_idx(lv_event_t *e){
//     uint8_t *inx_p = lv_event_get_param(e);
//     uint32_t index = 0;
//     memcpy(&index, inx_p, sizeof(uint32_t));
//     g_play_index = index;
//     ESP_LOGI(TAG, "INDEX:%ld", index);
// #ifndef SIMULATOR
//     if (!lv_obj_has_flag(vw.list_btn, LV_OBJ_FLAG_HIDDEN))
//     {
//         if (index>=g_loaded_start && index <= g_loaded_end)
//         {
//             for (uint32_t i = 0; i < g_loaded_end-g_loaded_start; i++)
//             {
//                 lv_obj_t *item = lv_obj_get_child(g_music_list, i);
//                 if (!item)
//                 {
//                     break;
//                 }
                
//                 if (i+g_loaded_start == index && !lv_obj_has_state(item, LV_STATE_CHECKED))
//                 {
//                     lv_obj_add_state(item, LV_STATE_CHECKED);
//                 }else if (i+g_loaded_start != index && lv_obj_has_state(item, LV_STATE_CHECKED))
//                 {
//                     lv_obj_remove_state(item, LV_STATE_CHECKED);
//                 }
//             }
            
//         }
          
//     }
// #endif
// }

// static lv_obj_t* _create_menu_cont(lv_obj_t *parent){
//     lv_obj_t *bg_mask = lv_obj_create(parent);
//     lv_obj_set_size(bg_mask, LV_HOR_RES, LV_VER_RES);
//     lv_obj_set_style_bg_color(bg_mask, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_bg_opa(bg_mask, LV_OPA_50, 0);
//     lv_obj_set_style_border_width(bg_mask, 0, 0);
//     lv_obj_clear_flag(bg_mask, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_add_flag(bg_mask, LV_OBJ_FLAG_HIDDEN);
//     lv_obj_add_event_cb(bg_mask, _on_bg_mask_click, LV_EVENT_ALL, NULL);

//     lv_obj_t *cont = lv_obj_create(parent);
//     lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
//     lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_radius(cont, 40, 0);
//     lv_obj_set_style_border_width(cont, 0, 0);
//     lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
//     lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_set_y(cont, 340);
//     lv_obj_set_x(cont, 0);
    
//     lv_obj_t *handle = lv_obj_create(cont);
//     lv_obj_set_size(handle, 48, 6);
//     lv_obj_set_style_radius(handle, 3, 0);
//     lv_obj_set_style_border_width(handle, 0, 0);
//     lv_obj_set_style_bg_color(handle, lv_color_hex(0x5A5A5A), 0);
//     lv_obj_set_style_bg_opa(handle, LV_OPA_60, 0);
//     lv_obj_align(handle, LV_ALIGN_TOP_MID, 0, -8);
//     lv_obj_add_flag(handle, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_clear_flag(handle, LV_OBJ_FLAG_SCROLLABLE);

//     lv_obj_t *seg_cont = lv_obj_create(cont);
//     lv_obj_set_size(seg_cont, 130, 46);
//     lv_obj_align(seg_cont, LV_ALIGN_TOP_MID, 0, 30);
//     lv_obj_set_style_bg_color(seg_cont, lv_color_hex(0x313133), 0);
//     lv_obj_set_style_border_width(seg_cont, 0, 0);
//     lv_obj_set_style_bg_opa(seg_cont, LV_OPA_COVER, 0);
//     lv_obj_clear_flag(seg_cont, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_set_style_radius(seg_cont, LV_RADIUS_CIRCLE, 0);
//     lv_obj_set_style_pad_all(seg_cont, 3, 0);

//     lv_obj_t *btn_mode_bt = lv_btn_create(seg_cont);
//     lv_obj_set_style_shadow_width(btn_mode_bt, 0, 0);
//     lv_obj_set_style_pad_all(btn_mode_bt, 0, 0);
//     lv_obj_set_style_radius(btn_mode_bt, LV_RADIUS_CIRCLE, 0);
//     lv_obj_align(btn_mode_bt, LV_ALIGN_LEFT_MID, 0, 0);
//     lv_obj_set_size(btn_mode_bt, 60, 40);
//     lv_obj_set_style_bg_color(btn_mode_bt, lv_color_hex(0x313133), 0);
//     lv_obj_set_style_bg_color(btn_mode_bt, lv_color_hex(0x5c5c5c), LV_PART_MAIN|LV_STATE_CHECKED);
//     lv_obj_t *img_bt = lv_img_create(btn_mode_bt);
//     lv_obj_center(img_bt);
//     lv_obj_add_event_cb(btn_mode_bt, _on_bt_btn_click, LV_EVENT_ALL, NULL);
//     lv_img_set_src(img_bt, &icon_bt_16);
    
//     lv_obj_t *btn_mode_tf = lv_btn_create(seg_cont);
//     lv_obj_set_style_shadow_width(btn_mode_tf, 0, 0);
//     lv_obj_set_style_pad_all(btn_mode_tf, 0, 0);
//     lv_obj_set_style_radius(btn_mode_tf, LV_RADIUS_CIRCLE, 0);
//     lv_obj_align(btn_mode_tf, LV_ALIGN_RIGHT_MID, 0, 0);
//     lv_obj_set_size(btn_mode_tf, 60, 40);
//     lv_obj_set_style_bg_color(btn_mode_tf, lv_color_hex(0x313133), 0);
//     lv_obj_set_style_bg_color(btn_mode_tf, lv_color_hex(0x5c5c5c), LV_PART_MAIN|LV_STATE_CHECKED);
//     lv_obj_set_style_bg_opa(btn_mode_tf, LV_OPA_0, LV_PART_MAIN|LV_STATE_DISABLED);
//     lv_obj_t *img_tf = lv_img_create(btn_mode_tf);
//     lv_obj_add_event_cb(btn_mode_tf, _on_tf_btn_click, LV_EVENT_ALL, NULL);
//     lv_obj_center(img_tf);
//     lv_img_set_src(img_tf, &icon_tf_16);
// #ifndef SIMULATOR
//     /*
//     vb_music_mode_t mode = vb_music_get_mode();
//     if (mode == VB_MUSIC_MODE_TF) {
//         lv_obj_add_state(btn_mode_tf, LV_STATE_CHECKED);
//     } else if (mode == VB_MUSIC_MODE_BT) {
//         lv_obj_add_state(btn_mode_bt, LV_STATE_CHECKED);
//     }
//     */
// #endif
//     vw.menu.cont_bg    = bg_mask;
//     vw.menu.cont       = cont;
//     vw.menu.seg_btn    = seg_cont;
//     vw.menu.open_y   = 240;
//     vw.menu.closed_y = 330;    
//     vw.menu.trig_bar = handle;
//     // 只在手柄上响应拖拽：把同一回调也挂到手柄即可
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_add_event_cb(cont, menu_drag_event_cb, LV_EVENT_ALL, &vw.menu);
//     lv_obj_add_event_cb(handle, menu_drag_event_cb, LV_EVENT_ALL, &vw.menu);

//     return cont;
// }

// #ifndef SIMULATOR
// static void _get_music_list_cb(uint32_t total, uint32_t index, uint32_t count, const char *list[]){
//     lvgl_port_lock(0);
//     music_list_feed(index, count, list, total);
//     lvgl_port_unlock();

// }

// static void my_music_request_cb(uint32_t start, uint32_t count, void *user_data)
// {
//     ESP_LOGI(TAG, "请求数据: start=%lu, count=%lu\n", start, count);
//     // vb_get_music_list_cb(start, count, _get_music_list_cb);
// }
// #else

// static const char *demo_all_names[] = {
//     "SONGS 1","SONGS 2","SONGS 3","SONGS 4","SONGS 5",
//     "SONGS 6","SONGS 7","SONGS 8","SONGS 9","SONGS 10",
//     "SONGS 11","SONGS 12","SONGS 13","SONGS 14","SONGS 15",
//     "SONGS 16","SONGS 17","SONGS 18","SONGS 19","SONGS 20",
//     "SONGS 21","SONGS 22","SONGS 23","SONGS 24","SONGS 25",
//  };

// static const uint32_t demo_total = sizeof(demo_all_names)/sizeof(demo_all_names[0]);

// uint32_t s_start = 0;
// uint32_t s_count = 0;
// static void onTimer(lv_timer_t* timer)
// {
//     uint32_t start = s_start;

//     uint32_t count = s_count;

//     if (start + count > demo_total) {
//         count = demo_total - start;
//     }

//     /* 这里可以是异步：先发 HTTP，等回来再调用 feed。
//        为演示方便，直接同步调用 feed。 */
//     const char *names[MUSIC_PAGE_SIZE];  // 最大一页5个
//     for (uint32_t i = 0; i < count; i++) {
//         names[i] = demo_all_names[start + i];
//     }
//     ESP_LOGI(TAG, "onTimer: start=%lu, count=%lu", start, count);
//     music_list_feed(start, count, names, demo_total);
// }

// static void my_music_request_cb(uint32_t start, uint32_t count, void *user_data)
// {
//     printf("请求数据: start=%lu, count=%lu\n", start, count);

//     if (start >= demo_total) {
//         music_list_feed(start, 0, NULL, demo_total);
//         return;
//     }

//     s_start = start;
//     s_count = count;

//     lv_timer_t* timer = lv_timer_create(onTimer, 100, (void *)start);
//     lv_timer_set_repeat_count(timer, 1);
// }
// #endif


// static void _on_btn_event_cb(lv_event_t *e){
//     if (!vw.is_act) {
//         return;
//     }

//     lv_obj_t *obj = lv_event_get_current_target(e);
//     if(obj == vw.list_btn){
//         lv_obj_clear_flag(vw.list_cont, LV_OBJ_FLAG_HIDDEN);
//         if (g_loaded_end == 0 && g_loaded_start == 0)
//         {
// #ifndef SIMULATOR
//             // g_play_index = vb_get_play_index();
// #endif
//             request_page(g_play_index);
//         }
//     }
// #ifndef SIMULATOR
//     /*
//     else if (obj == vw.play_btn)
//     {
//         vb_music_mode_t mode = vb_music_get_mode();
//         uint8_t playing = vb_music_get_play_status();
//         lv_obj_t *img = lv_obj_get_child(vw.play_btn, 0);
//         if(playing == 1){
//             vb_music_play(0);
//             if (mode == VB_MUSIC_MODE_TF)
//             {
//                 lv_img_set_src(img, &icon_play_32);
//             }
//         }
//         else{
//             vb_music_play(1);
//             if (mode == VB_MUSIC_MODE_TF)
//             {
//                 lv_img_set_src(img, &icon_pause_32);
//             }
//         }
//     }else if (obj == vw.next_btn)
//     {
//         vb_music_next_priv(1);
//     }else if (obj == vw.priv_btn)
//     {
//         vb_music_next_priv(0);
//     }
//     */
// #endif
// }

// static void _on_title(lv_event_t *e){
//     lv_obj_t *title = lv_event_get_user_data(e);
//     char *str = lv_event_get_param(e);
//     lv_label_set_text(title, str);

// }

// static void _on_lyrc(lv_event_t *e){
//     lv_obj_t *lyrc = lv_event_get_user_data(e);
//     char *str = lv_event_get_param(e);
//     lv_label_set_text(lyrc, str);
// }

// static void _on_time(lv_event_t *e){
    
//     uint8_t *time8 = lv_event_get_param(e);
//     uint32_t time32[2] = {0};
//     memcpy(time32, time8, sizeof(time32));
//     lv_label_set_text_fmt(vw.time_curr, "%02d:%02d", (int)time32[0]/60, (int)time32[0]%60);
//     lv_label_set_text_fmt(vw.time_total, "%02d:%02d", (int)time32[1]/60, (int)time32[1]%60);
//     uint8_t percent = 100;
//     if(time32[1] != 0){
//         percent = (time32[0]*100/time32[1]);
//     }
//     lv_slider_set_value(vw.progress_bar, percent, LV_ANIM_OFF);
// }

// static void _on_gesture_cb(lv_event_t *e){
//     static int x = 0;
//     if (!vw.is_act){
//         return;
//     }

//     lv_event_t* event = lv_event_get_param(e);
//     lv_event_code_t code = lv_event_get_code(event);
//     if (code == LV_EVENT_GESTURE)
//     {
//         lv_indev_wait_release(lv_indev_active());
//         lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
//         switch (dir)
//         {
//         case LV_DIR_RIGHT:
//         case LV_DIR_LEFT:
//             if(!lv_obj_has_flag(vw.list_cont, LV_OBJ_FLAG_HIDDEN)){
//                 lv_obj_add_flag(vw.list_cont, LV_OBJ_FLAG_HIDDEN);
//             }else{
//                 // page_change("home");
//             }
//             break;
//         default:
//             break;
//         }
//     }
// }

// static void _on_btn_cb(lv_event_t *e)
// {
//     if (!vw.is_act) return;

//     cl_button_t *btn = (cl_button_t*)lv_event_get_param(e);
//     if (!btn) return;

//     switch (btn->id)
//     {
//         case CL_UI_KEY_MODE:
//             if (btn->event == CL_BTN_CLICK)
//             {
//                 page_change("home");
//             }    
//             break;
//         default:
//             break;
//     }
// }

// music_view_t* music_view_create(lv_obj_t* root) {

//     lv_obj_remove_style_all(root);
//     lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);     // 背景白色
//     lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
//     lv_obj_set_style_border_width(root, 0, 0);
//     lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
//     lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

//     // 播放页面
//     lv_obj_t *player_cont = music_player_create(root);
//     vw.player_cont = player_cont;

//     lv_obj_t *list_cont = music_list_create(root, my_music_request_cb, NULL);
//     vw.list_cont = list_cont;
//     lv_obj_add_flag(vw.list_cont, LV_OBJ_FLAG_HIDDEN);
    
//     _create_menu_cont(root);


//     lv_obj_add_event_cb(vw.list_btn, _on_btn_event_cb, LV_EVENT_CLICKED, NULL);
// //     lv_obj_add_event_cb(vw.loop_btn, _on_btn_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);

//     lv_obj_add_event_cb(vw.play_btn, _on_btn_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(vw.priv_btn, _on_btn_event_cb, LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(vw.next_btn, _on_btn_event_cb, LV_EVENT_CLICKED, NULL);

//     lv_obj_add_event_cb(root, _on_gesture_cb, LV_EVENT_GESTURE, NULL);
//     lv_obj_add_event_cb(root, _on_mode_change, CL_UI_EVENT_MODE_CHANGE, NULL);
//     lv_obj_add_event_cb(root, _on_play_status, CL_UI_EVENT_MUSIC_STATUS, NULL);
//     lv_obj_add_event_cb(root, _on_music_idx, CL_UI_EVENT_MUSIC_IDX, NULL);
    
//     lv_obj_add_event_cb(root, _on_title, CL_UI_EVENT_MUSIC_TITLE, vw.title_label);
//     lv_obj_add_event_cb(root, _on_lyrc, CL_UI_EVENT_MUSIC_LYRC, vw.lyrc_label);
//     lv_obj_add_event_cb(root, _on_time, CL_UI_EVENT_MUSIC_TIME, NULL);


//     return &vw;
// }


// void music_view_delete(void){
//     g_req_in_progress = false;
//     g_total_count = 0;
//     g_loaded_start = 0;
//     g_loaded_end = 0;
//     g_req_tick = 0;
//     g_req_dir = REQ_NONE;
//     g_req_start = 0;
//     g_req_count = 0;
//     g_anchor_index = UINT32_MAX;
//     g_req_cb = NULL;
//     g_req_user_data = NULL;
// }