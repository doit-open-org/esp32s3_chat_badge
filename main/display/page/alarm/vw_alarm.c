#include "vw_alarm.h"
#include "lvgl.h"
#include "cl_ui.h"
#include "services/alarm/xiaozhi_alarm.h"
static alarm_view_t vw;
LV_FONT_DECLARE(font_puhui_20_4)
LV_FONT_DECLARE(font_puhui_num_32_4)
LV_IMG_DECLARE(add)
LV_IMG_DECLARE(bottom_btn)
LV_IMG_DECLARE(icon_right)

typedef struct {
    xz_alarm_info_t *info;   
    lv_obj_t * cont;      // 容器
    lv_obj_t * time_label;
    lv_obj_t * week_label;
    lv_obj_t * sw;
} alarm_item_t;

static alarm_item_t alarms[10]; // 两个闹钟

#define TAG "xiaozhi_alarm_view"

static void _on_alarm_item_click(lv_event_t * e);
static void _on_alarm_sw_click(lv_event_t * e);

// 创建一个闹钟行
static void create_alarm(lv_obj_t * parent, alarm_item_t * alarm, xz_alarm_info_t *info)
{
    // 创建容器，布局水平排列，宽度和父容器一样
    alarm->cont = lv_obj_create(parent);
    lv_obj_set_size(alarm->cont, 220, 50);
    lv_obj_set_style_pad_all(alarm->cont, 0, 0);
    lv_obj_set_style_border_width(alarm->cont, 0, 0);
    lv_obj_set_style_bg_color(alarm->cont, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(alarm->cont, LV_OBJ_FLAG_SCROLLABLE);

    // 时间label - 左边
    alarm->time_label = lv_label_create(alarm->cont);
    lv_label_set_text_fmt(alarm->time_label, "%02d:%02d", info->hour, info->min);
    lv_obj_set_style_text_font(alarm->time_label, &font_puhui_num_32_4, 0);
    lv_obj_set_style_text_color(alarm->time_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(alarm->time_label, LV_ALIGN_TOP_LEFT, 0, 0);

    char week_str[32] = {0};
    char *week_day[7] = {"一","二","三","四","五","六","日"};
    if (info->week_mask == 0x7f)
    {
        sprintf(week_str, "每天");
    }else if (info->week_mask == 0)
    {
        sprintf(week_str, "单次");
    }else{
        for (uint8_t i = 0; i < 7; i++)
        {
            if (((info->week_mask >> i) & 0x01) == 1)
            {
                strcat(week_str, week_day[i]);
            }
        }
    }
    
    

    alarm->week_label = lv_label_create(alarm->cont);
    lv_label_set_text(alarm->week_label, week_str);
    lv_obj_set_style_text_color(alarm->week_label, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(alarm->week_label, &font_puhui_20_4, 0);
    lv_obj_align(alarm->week_label, LV_ALIGN_BOTTOM_LEFT, 0, 4);

    alarm->info = info;

    // Switch - 右边
    alarm->sw = lv_switch_create(alarm->cont);
    lv_obj_set_size(alarm->sw, 50, 25);
    lv_obj_align(alarm->sw, LV_ALIGN_RIGHT_MID, 0, 0);

    if (alarm->info->onoff)
    {
        lv_obj_add_state(alarm->sw, LV_STATE_CHECKED);
    }

    lv_obj_add_flag(alarm->cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(alarm->cont, _on_alarm_item_click, LV_EVENT_CLICKED, alarm);
    lv_obj_add_event_cb(alarm->sw, _on_alarm_sw_click, LV_EVENT_CLICKED, alarm);

}

static bool _get_alarm_map(){
    if (vw.alarm_info)
    {
        free(vw.alarm_info);
        vw.alarm_info = NULL;
    }
    vw.alarm_num = xiaozhi_alarm_get_num();
    if (vw.alarm_num != 0)
    {
        vw.alarm_info = malloc(sizeof(xz_alarm_info_t)*vw.alarm_num);
        memset(vw.alarm_info, 0, sizeof(xz_alarm_info_t)*vw.alarm_num);
        return xiaozhi_get_alarm_info(vw.alarm_info, &vw.alarm_num);
    }
    return 0;
}

static void _refresh_alarm_list(){
    // 删除vw.list_cont下所有子控件
    lv_obj_t *child = lv_obj_get_child(vw.list_cont, 0);
    while (child) {
        lv_obj_del(child);
        child = lv_obj_get_child(vw.list_cont, 0);;
    }
    
    if (_get_alarm_map())
    {
        for (uint8_t i = 0; i < vw.alarm_num; i++)
        {
            create_alarm(vw.list_cont, &alarms[i], &vw.alarm_info[i]);
        }
    }
    

}

// 创建星期按钮
static lv_obj_t * _create_week_btn(lv_obj_t * comp_parent, const char * week_name)
{

    lv_obj_t * cui_btnweek;
    cui_btnweek = lv_button_create(comp_parent);
    lv_obj_set_width(cui_btnweek, 60);
    lv_obj_set_height(cui_btnweek, 60);
    lv_obj_set_align(cui_btnweek, LV_ALIGN_CENTER);
    lv_obj_add_flag(cui_btnweek, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_remove_flag(cui_btnweek, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(cui_btnweek, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_btnweek, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_btnweek, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cui_btnweek, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cui_btnweek, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_btnweek, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_btnweek, lv_color_hex(0x00ADD1), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(cui_btnweek, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(cui_btnweek, 0, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(cui_btnweek, 0, 0);
    lv_obj_set_style_shadow_opa(cui_btnweek, LV_OPA_TRANSP, 0);


    lv_obj_t * cui_label_week;
    cui_label_week = lv_label_create(cui_btnweek);
    lv_obj_set_width(cui_label_week, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(cui_label_week, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(cui_label_week, LV_ALIGN_CENTER);
    lv_label_set_text(cui_label_week, week_name);
    lv_obj_set_style_text_font(cui_label_week, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    return cui_btnweek;
}

static void _on_alarm_item_click(lv_event_t * e){
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    alarm_item_t *alarm = (alarm_item_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED)
    {
        lv_label_set_text_fmt(vw.label_alarm_info_time, "%02d:%02d", alarm->info->hour, alarm->info->min);
        char week_str[32] = {0};
        char *week_day[7] = {"一","二","三","四","五","六","日"};
        if (alarm->info->week_mask == 0x7f)
        {
            sprintf(week_str, "每天");
        }else if (alarm->info->week_mask == 0)
        {
            sprintf(week_str, "单次");
        }else{
            for (uint8_t i = 0; i < 7; i++)
            {
                if (((alarm->info->week_mask >> i) & 0x01) == 1)
                {
                    strcat(week_str, week_day[i]);
                }
            }
        }
        lv_label_set_text(vw.label_alarm_info_week, week_str);
        vw.curr_edit_index = alarm->info->index;
        _refresh_alarm_list();
        vw.curr_mode = ALARM_MODE_EDIT;
        lv_obj_clear_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
    }
}

static void _on_alarm_sw_click(lv_event_t * e){
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    alarm_item_t *alarm = (alarm_item_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED)
    {
       xz_alarm_info_t *alarm_info = alarm->info;
        if (lv_obj_has_state(obj, LV_STATE_CHECKED))
        {
            // 开启闹钟
            alarm_info->onoff = 1;
            // LOGI("Alarm %d ON", alarm_info->index);
        }else{
            // 关闭闹钟
            alarm_info->onoff = 0;
            // LOGI("Alarm %d OFF", alarm_info->index);
        }
        xiaozhi_alarm_set(alarm_info, alarm_info->index);
    }
}

static void _on_time_set_cb(lv_event_t * e){
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        xz_alarm_info_t *info = NULL;
        for (uint8_t i = 0; i < vw.alarm_num; i++)
        {
            if (vw.curr_edit_index == vw.alarm_info[i].index)
            {
                info = &vw.alarm_info[i];
                break;
            }
        }
        if (info == NULL)
        {
            // LOGE("alarm info not found for index %d", vw.curr_edit_index);
            lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        lv_roller_set_selected(vw.roller_hour, info->hour, LV_ANIM_OFF);
        lv_roller_set_selected(vw.roller_min, info->min, LV_ANIM_OFF);
        lv_obj_t *label = lv_obj_get_child(vw.btn_time_set, 0);
        lv_label_set_text(label, "确定");
        lv_obj_clear_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
    }
}

static void _on_week_set_cb(lv_event_t * e){
    lv_obj_t * obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        xz_alarm_info_t *info = NULL;
        for (uint8_t i = 0; i < vw.alarm_num; i++)
        {
            if (vw.curr_edit_index == vw.alarm_info[i].index)
            {
                info = &vw.alarm_info[i];
                break;
            }
        }
        if (info == NULL)
        {
            // LOGE("alarm info not found for index %d", vw.curr_edit_index);
            lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        for (uint8_t i = 0; i < 7; i++)
        {
            if ((info->week_mask >> i) & 0x01)
            {
                lv_obj_add_state(vw.btn_week[i], LV_STATE_CHECKED);
            }else{
                lv_obj_clear_state(vw.btn_week[i], LV_STATE_CHECKED);
            }
        }
        lv_obj_clear_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
    }
}

static void _on_next_btn_cb(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED){
        if (vw.curr_mode == ALARM_MODE_ADD)
        {        
            lv_obj_clear_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
        }else if (vw.curr_mode == ALARM_MODE_EDIT)
        {
            xz_alarm_info_t *alarm_info = NULL;
            for (uint8_t i = 0; i < vw.alarm_num; i++)
            {
                if (vw.curr_edit_index == vw.alarm_info[i].index)
                {
                    alarm_info = &vw.alarm_info[i];
                    break;
                }
            }
            if (alarm_info == NULL)
            {
                // LOGE("alarm info not found for index %d", vw.curr_edit_index);
                lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
                return;
            }
            alarm_info->onoff = 1;
            alarm_info->hour = lv_roller_get_selected(vw.roller_hour);
            alarm_info->min = lv_roller_get_selected(vw.roller_min);
            xiaozhi_alarm_add(alarm_info);
            lv_label_set_text_fmt(vw.label_alarm_info_time, "%02d:%02d", alarm_info->hour, alarm_info->min);
            lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
        }else{
            lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void _on_del_btn_cb(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED){
    
        xz_alarm_info_t *info = NULL;
        for (uint8_t i = 0; i < vw.alarm_num; i++)
        {
            if (vw.curr_edit_index == vw.alarm_info[i].index)
            {
                info = &vw.alarm_info[i];
                break;
            }
        }
        if (info == NULL)
        {
            // LOGE("alarm info not found for index %d", vw.curr_edit_index);
            lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        xiaozhi_alarm_del(info->index);
        _refresh_alarm_list();
        lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
        vw.curr_mode = ALARM_MODE_IDLE;
    }
}

static void _on_submit_btn_cb(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED){
        if (vw.curr_mode == ALARM_MODE_ADD)
        {
            uint8_t index = xiaozhi_alarm_get_free_alarm_index();
            
            xz_alarm_info_t alarm_info = {
                .hour = lv_roller_get_selected(vw.roller_hour),
                .min = lv_roller_get_selected(vw.roller_min),
                .onoff = 1,
                .index = index,
                .week_mask = 0
            };
            for (uint8_t i = 0; i < 7; i++)
            {
                if (lv_obj_has_state(vw.btn_week[i], LV_STATE_CHECKED))
                {
                    alarm_info.week_mask |= (0x01<<i);
                }
            }
            xiaozhi_alarm_add(&alarm_info);
            _refresh_alarm_list();
            vw.curr_mode = ALARM_MODE_IDLE;
            lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
        }else if (vw.curr_mode == ALARM_MODE_EDIT){
            // 编辑模式
            xz_alarm_info_t *alarm_info = NULL;
            for (uint8_t i = 0; i < vw.alarm_num; i++)
            {
                if (vw.curr_edit_index == vw.alarm_info[i].index)
                {
                    alarm_info = &vw.alarm_info[i];
                    break;
                }
            }
            if (alarm_info == NULL)
            {
                // LOGE("alarm info not found for index %d", vw.curr_edit_index);
                lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
                return;
            }


            alarm_info->onoff = 1;
            alarm_info->week_mask = 0;

            for (uint8_t i = 0; i < 7; i++)
            {
                if (lv_obj_has_state(vw.btn_week[i], LV_STATE_CHECKED))
                {
                    alarm_info->week_mask |= (0x01<<i);
                }
            }

            char week_str[32] = {0};
            char *week_day[7] = {"一","二","三","四","五","六","日"};
            if (alarm_info->week_mask == 0x7f)
            {
                sprintf(week_str, "每天");
            }else if (alarm_info->week_mask == 0)
            {
                sprintf(week_str, "单次");
            }else{
                for (uint8_t i = 0; i < 7; i++)
                {
                    if (((alarm_info->week_mask >> i) & 0x01) == 1)
                    {
                        strcat(week_str, week_day[i]);
                    }
                }
            }
            lv_label_set_text(vw.label_alarm_info_week, week_str);
            lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
            xiaozhi_alarm_add(alarm_info);
        }
    }
}

static void _on_add_btn_cb(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    // LOGD("on add btn cb");
    if (code == LV_EVENT_CLICKED)
    {
        vw.curr_mode = ALARM_MODE_ADD;
        for (uint8_t i = 0; i < 7; i++)
        {
            lv_obj_clear_state(vw.btn_week[i], LV_STATE_CHECKED);
        }
        lv_obj_t *label = lv_obj_get_child(vw.btn_time_set, 0);
        lv_label_set_text(label, "下一步");
        // cl_time_t time = ui_tool_get_time();
        // lv_roller_set_selected(vw.roller_hour, time.hour, LV_ANIM_OFF);
        // lv_roller_set_selected(vw.roller_min, time.min, LV_ANIM_OFF);
        lv_obj_clear_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
    }
}

static void _on_btn_cb(lv_event_t *e){
    if (!vw.is_act)
    {
        return;
    }
    
    cl_button_t *btn = (cl_button_t*)lv_event_get_param(e);
    if (!btn) return;
    switch (btn->id)
    {
    case CL_UI_KEY_MODE:
        if (btn->event == CL_BTN_CLICK)
        {
            if (!lv_obj_has_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN))
            {
                lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);
            }else if (!lv_obj_has_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN))
            {
                lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
                if (vw.curr_mode == ALARM_MODE_ADD)
                {
                    vw.curr_mode = ALARM_MODE_IDLE;
                }            
            }else if (!lv_obj_has_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN))
            {
                _refresh_alarm_list();
                lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
                vw.curr_mode = ALARM_MODE_IDLE;
            }else{
                page_change("home");
            }
        }
        break;
    default:
        break;
    }
    
}

static lv_obj_t *_create_alarm_info_cont(lv_obj_t *root)
{
    lv_obj_t *cont_main = lv_obj_create(root);
    lv_obj_remove_style_all(cont_main);  // 清除默认样式
    lv_obj_set_size(cont_main, LV_HOR_RES, LV_VER_RES);  // 设置容器大小为屏幕大小
    lv_obj_set_style_bg_opa(cont_main, LV_OPA_COVER, 0);              // 设置背景不透明度为不透明
    lv_obj_set_style_border_width(cont_main, 0, 0);                   // 清除边框
    lv_obj_set_style_pad_all(cont_main, 0, 0);                         // 清除内边距
    lv_obj_remove_flag(cont_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(cont_main);    
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(0x000000), 0);  // 设置背景色为黑色
    
    lv_obj_t *label_alarm_edit = lv_label_create(cont_main);
    lv_obj_set_width(label_alarm_edit, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_alarm_edit, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label_alarm_edit, 0);
    lv_obj_set_y(label_alarm_edit, 40);
    lv_obj_set_align(label_alarm_edit, LV_ALIGN_TOP_MID);
    lv_label_set_text(label_alarm_edit, "编辑闹钟");
    lv_obj_set_style_text_color(label_alarm_edit, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(label_alarm_edit, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 时间容器
    lv_obj_t *cont_time_info = lv_obj_create(cont_main);
    lv_obj_remove_style_all(cont_time_info);
    lv_obj_set_width(cont_time_info, 300);
    lv_obj_set_height(cont_time_info, 60);
    lv_obj_set_x(cont_time_info, 0);
    lv_obj_set_y(cont_time_info, -40);
    lv_obj_set_align(cont_time_info, LV_ALIGN_CENTER);
    lv_obj_remove_flag(cont_time_info, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    vw.label_alarm_info_time = lv_label_create(cont_time_info);
    lv_obj_set_width(vw.label_alarm_info_time, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(vw.label_alarm_info_time, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(vw.label_alarm_info_time, 0);
    lv_obj_set_y(vw.label_alarm_info_time, -10);
    lv_obj_set_align(vw.label_alarm_info_time, LV_ALIGN_CENTER);
    // lv_label_set_text_fmt(vw.label_alarm_info_time, "%02d:%02d", vw.alarm_info[vw.curr_edit_index].hour, vw.alarm_info[vw.curr_edit_index].min);
    lv_obj_set_style_text_color(vw.label_alarm_info_time, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(vw.label_alarm_info_time, &font_puhui_num_32_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label_change_time = lv_label_create(cont_time_info);
    lv_obj_set_width(label_change_time, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_change_time, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label_change_time, 0);
    lv_obj_set_y(label_change_time, 18);
    lv_obj_set_align(label_change_time, LV_ALIGN_CENTER);
    lv_label_set_text(label_change_time, "修改时间");
    lv_obj_set_style_text_color(label_change_time, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_change_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_change_time, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *icon_arrow_right = lv_image_create(cont_time_info);
    lv_image_set_src(icon_arrow_right, &icon_right);
    lv_obj_set_width(icon_arrow_right, LV_SIZE_CONTENT);   /// 32
    lv_obj_set_height(icon_arrow_right, LV_SIZE_CONTENT);    /// 32
    lv_obj_set_x(icon_arrow_right, 120);
    lv_obj_set_y(icon_arrow_right, 0);
    lv_obj_set_align(icon_arrow_right, LV_ALIGN_CENTER);
    lv_obj_add_flag(icon_arrow_right, LV_OBJ_FLAG_CLICKABLE);     /// Flags
    lv_obj_remove_flag(icon_arrow_right, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    lv_obj_add_flag(cont_time_info, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_time_info, _on_time_set_cb, LV_EVENT_CLICKED, NULL);

    // 周容器
    lv_obj_t *week_cont = lv_obj_create(cont_main);
    lv_obj_remove_style_all(week_cont);
    lv_obj_set_width(week_cont, 300);
    lv_obj_set_height(week_cont, 60);
    lv_obj_set_x(week_cont, 0);
    lv_obj_set_y(week_cont, 40);
    lv_obj_set_align(week_cont, LV_ALIGN_CENTER);
    lv_obj_remove_flag(week_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    vw.label_alarm_info_week = lv_label_create(week_cont);
    lv_obj_set_width(vw.label_alarm_info_week, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(vw.label_alarm_info_week, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(vw.label_alarm_info_week, 0);
    lv_obj_set_y(vw.label_alarm_info_week, -10);
    lv_obj_set_align(vw.label_alarm_info_week, LV_ALIGN_CENTER);
    lv_label_set_text(vw.label_alarm_info_week, " ");
    lv_obj_set_style_text_color(vw.label_alarm_info_week, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(vw.label_alarm_info_week, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label_repeat= lv_label_create(week_cont);
    lv_obj_set_width(label_repeat, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_repeat, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label_repeat, 0);
    lv_obj_set_y(label_repeat, 18);
    lv_obj_set_align(label_repeat, LV_ALIGN_CENTER);
    lv_label_set_text(label_repeat, "重复");
    lv_obj_set_style_text_color(label_repeat, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_repeat, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_repeat, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    icon_arrow_right = lv_image_create(week_cont);
    lv_image_set_src(icon_arrow_right, &icon_right);
    lv_obj_set_width(icon_arrow_right, LV_SIZE_CONTENT);   /// 32
    lv_obj_set_height(icon_arrow_right, LV_SIZE_CONTENT);    /// 32
    lv_obj_set_x(icon_arrow_right, 120);
    lv_obj_set_y(icon_arrow_right, 0);
    lv_obj_set_align(icon_arrow_right, LV_ALIGN_CENTER);
    lv_obj_add_flag(icon_arrow_right, LV_OBJ_FLAG_CLICKABLE);     /// Flags
    lv_obj_remove_flag(icon_arrow_right, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_add_flag(week_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(week_cont, _on_week_set_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_delete = lv_img_create(cont_main);
    lv_img_set_src(btn_delete, &bottom_btn);
    lv_obj_align(btn_delete, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_add_flag(btn_delete, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label_next;
    label_next = lv_label_create(btn_delete);
    lv_obj_set_width(label_next, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_next, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(label_next, LV_ALIGN_CENTER);
    lv_label_set_text(label_next, "删除");
    lv_obj_set_style_text_color(label_next, lv_color_hex(0xff5353), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_next, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_next, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);
   
    lv_obj_add_event_cb(btn_delete, _on_del_btn_cb, LV_EVENT_CLICKED, NULL);
    return cont_main;
}

static lv_obj_t *_create_list_cont(lv_obj_t *root)
{
 // 在 root 下创建一个容器，并设置背景色
    lv_obj_t *cont_list = lv_obj_create(root);
    lv_obj_remove_style_all(cont_list);  // 清除默认样式
    lv_obj_set_size(cont_list, LV_HOR_RES, LV_VER_RES);  // 设置容器大小为屏幕大小
    lv_obj_set_style_bg_opa(cont_list, LV_OPA_COVER, 0);              // 设置背景不透明度为不透明
    lv_obj_set_style_border_width(cont_list, 0, 0);                   // 清除边框
    lv_obj_set_style_pad_all(cont_list, 0, 0);                         // 清除内边距
    lv_obj_remove_flag(cont_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(cont_list);    
    lv_obj_set_style_bg_color(cont_list, lv_color_hex(0x000000), 0);  // 设置背景色为黑色
    
    
    vw.list_cont = lv_obj_create(cont_list);
    lv_obj_align(vw.list_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(vw.list_cont, LV_HOR_RES, 300);
    lv_obj_set_style_border_width(vw.list_cont, 0, 0);  
    lv_obj_set_style_bg_color(vw.list_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(vw.list_cont, 0, 0);
    lv_obj_set_scrollbar_mode(vw.list_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(vw.list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(vw.list_cont, 35,0);
    lv_obj_set_flex_align(vw.list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(vw.list_cont, LV_DIR_VER);  // 设置滚动方向为垂直

    vw.btn_add = lv_button_create(cont_list);
    lv_obj_set_width(vw.btn_add, 60);
    lv_obj_set_height(vw.btn_add, 60);
    lv_obj_set_x(vw.btn_add, 0);
    lv_obj_set_y(vw.btn_add, -20);
    lv_obj_set_align(vw.btn_add, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(vw.btn_add, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_remove_flag(vw.btn_add, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(vw.btn_add, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(vw.btn_add, 0, 0);
    lv_obj_set_style_shadow_opa(vw.btn_add, LV_OPA_TRANSP, 0);

    lv_obj_add_event_cb(vw.btn_add, _on_add_btn_cb, LV_EVENT_CLICKED, NULL);
    

    lv_obj_t *ui_Image3 = lv_image_create(vw.btn_add);
    lv_image_set_src(ui_Image3, &add);
    lv_obj_set_width(ui_Image3, LV_SIZE_CONTENT);   /// 32
    lv_obj_set_height(ui_Image3, LV_SIZE_CONTENT);    /// 32
    lv_obj_set_align(ui_Image3, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image3, LV_OBJ_FLAG_CLICKABLE);     /// Flags
    lv_obj_remove_flag(ui_Image3, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    
    return cont_list;
}

static lv_obj_t *_create_alarm_time_set_cont(lv_obj_t *root){
    lv_obj_t *cont_main = lv_obj_create(root);
    lv_obj_remove_style_all(cont_main);  // 清除默认样式
    lv_obj_set_size(cont_main, LV_HOR_RES, LV_VER_RES);  // 设置容器大小为屏幕大小
    lv_obj_set_style_bg_opa(cont_main, LV_OPA_COVER, 0);              // 设置背景不透明度为不透明
    lv_obj_set_style_border_width(cont_main, 0, 0);                   // 清除边框
    lv_obj_set_style_pad_all(cont_main, 0, 0);                         // 清除内边距
    lv_obj_remove_flag(cont_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(cont_main);    
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(0x000000), 0);  // 设置背景色为黑色
    
    lv_obj_t * label_time_change = lv_label_create(cont_main);
    lv_obj_set_width(label_time_change, LV_SIZE_CONTENT); 
    lv_obj_set_height(label_time_change, LV_SIZE_CONTENT); 
    lv_obj_align(label_time_change, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(label_time_change, "时间设置");
    lv_obj_set_style_text_color(label_time_change, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(label_time_change, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    vw.roller_hour = lv_roller_create(cont_main);
    lv_roller_set_options(vw.roller_hour,
                          "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24",
                          LV_ROLLER_MODE_INFINITE);
    lv_obj_set_height(vw.roller_hour, 183);
    lv_obj_set_width(vw.roller_hour, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(vw.roller_hour, -50);
    lv_obj_set_y(vw.roller_hour, 0);
    lv_obj_set_align(vw.roller_hour, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(vw.roller_hour, &font_puhui_num_32_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(vw.roller_hour, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(vw.roller_hour, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(vw.roller_hour, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(vw.roller_hour, lv_color_hex(0x00ADD1), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(vw.roller_hour, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(vw.roller_hour, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(vw.roller_hour, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);

    vw.roller_min = lv_roller_create(cont_main);
    lv_roller_set_options(vw.roller_min,
                          "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59",
                          LV_ROLLER_MODE_INFINITE);
    lv_obj_set_height(vw.roller_min, 183);
    lv_obj_set_width(vw.roller_min, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(vw.roller_min, 50);
    lv_obj_set_y(vw.roller_min, 0);
    lv_obj_set_align(vw.roller_min, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(vw.roller_min, &font_puhui_num_32_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(vw.roller_min, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(vw.roller_min, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(vw.roller_min, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(vw.roller_min, lv_color_hex(0x00ADD1), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(vw.roller_min, 255, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(vw.roller_min, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(vw.roller_min, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);

    lv_obj_t *btn_next = lv_img_create(cont_main);
    vw.btn_time_set = btn_next;
    lv_img_set_src(btn_next, &bottom_btn);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_add_flag(btn_next, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label_next;
    label_next = lv_label_create(btn_next);
    lv_obj_set_width(label_next, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_next, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(label_next, LV_ALIGN_CENTER);
    lv_label_set_text(label_next, "下一步");
    lv_obj_set_style_text_color(label_next, lv_color_hex(0x00ADD1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_next, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_next, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_add_event_cb(btn_next, _on_next_btn_cb, LV_EVENT_CLICKED, NULL);
    return cont_main;
}

static lv_obj_t *_create_alarm_week_set_cont(lv_obj_t *root){
    lv_obj_t *cont_main = lv_obj_create(root);
    lv_obj_remove_style_all(cont_main);  // 清除默认样式
    lv_obj_set_size(cont_main, LV_HOR_RES, LV_VER_RES);  // 设置容器大小为屏幕大小
    lv_obj_set_style_bg_opa(cont_main, LV_OPA_COVER, 0);              // 设置背景不透明度为不透明
    lv_obj_set_style_border_width(cont_main, 0, 0);                   // 清除边框
    lv_obj_set_style_pad_all(cont_main, 0, 0);                         // 清除内边距
    lv_obj_remove_flag(cont_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(cont_main);    
    lv_obj_set_style_bg_color(cont_main, lv_color_hex(0x000000), 0);  // 设置背景色为黑色
    
    lv_obj_t *label_repeat = lv_label_create(cont_main);
    lv_obj_set_width(label_repeat, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_repeat, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(label_repeat, 0);
    lv_obj_set_y(label_repeat, 40);
    lv_obj_set_align(label_repeat, LV_ALIGN_TOP_MID);
    lv_label_set_text(label_repeat, "重复");
    lv_obj_set_style_text_color(label_repeat, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(label_repeat, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < 7; i++)
    {
        vw.btn_week[i] = _create_week_btn(cont_main, (const char*[]){"一","二","三","四","五","六","日"}[i]);
    }
    lv_obj_align(vw.btn_week[0], LV_ALIGN_CENTER, -60, -60);
    lv_obj_align(vw.btn_week[1], LV_ALIGN_CENTER, 60, -60);
    lv_obj_align(vw.btn_week[2], LV_ALIGN_CENTER, -90, 10);
    lv_obj_align(vw.btn_week[3], LV_ALIGN_CENTER, 0, 10);
    lv_obj_align(vw.btn_week[4], LV_ALIGN_CENTER, 90, 10);
    lv_obj_align(vw.btn_week[5], LV_ALIGN_CENTER, -60, 80);
    lv_obj_align(vw.btn_week[6], LV_ALIGN_CENTER, 60, 80);

    lv_obj_t *btn_next = lv_img_create(cont_main);
    lv_img_set_src(btn_next, &bottom_btn);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_add_flag(btn_next, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * label_next;
    label_next = lv_label_create(btn_next);
    lv_obj_set_width(label_next, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(label_next, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(label_next, LV_ALIGN_CENTER);
    lv_label_set_text(label_next, "确定");
    lv_obj_set_style_text_color(label_next, lv_color_hex(0x00ADD1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label_next, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label_next, &font_puhui_20_4, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_add_event_cb(btn_next, _on_submit_btn_cb, LV_EVENT_CLICKED, NULL);
    return cont_main;
}

alarm_view_t* alarm_view_create(lv_obj_t *root)
{
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
 
    vw.curr_mode = ALARM_MODE_IDLE;
    vw.cont_alarm_list = _create_list_cont(root);
    vw.cont_alarm_info = _create_alarm_info_cont(root);
    vw.cont_alarm_time_set = _create_alarm_time_set_cont(root);
    vw.cont_alarm_week_set = _create_alarm_week_set_cont(root);

    lv_obj_add_flag(vw.cont_alarm_info, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(vw.cont_alarm_time_set, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(vw.cont_alarm_week_set, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(root, _on_btn_cb, CL_UI_EVENT_BUTTON, NULL);
    
    _refresh_alarm_list();
       
    return &vw;
}

void alarm_view_delete(void)
{
    if (vw.alarm_info != NULL)
    {
        free(vw.alarm_info);
        vw.alarm_info = NULL;
    }
    
}

void alarm_view_appear_anim_start(bool reverse)
{
    // 如果需要反向动画，可以在这里实现
}
