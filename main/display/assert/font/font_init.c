#include "lvgl.h"
#include <stdint.h>
#include "esp_log.h"

#define TAG "font_init"

// 声明外部字体
LV_FONT_DECLARE(font_puhui_20_4)
LV_FONT_DECLARE(font_puhui_num_32_4)
LV_FONT_DECLARE(font_puhui_ascii_80_4)

// 全局字体变量
lv_font_t g_font_ext_puhui_20_4;
lv_font_t g_font_ext_puhui_28_4;

uint8_t font_ext_puhui_20_4_init() {
    return 0;
}

void cl_font_init() {
    ESP_LOGI(TAG, "Starting font initialization...");
    
    extern const lv_font_t BUILTIN_TEXT_FONT;
    g_font_ext_puhui_20_4 = BUILTIN_TEXT_FONT;
    g_font_ext_puhui_28_4 = BUILTIN_TEXT_FONT;
    
    ESP_LOGI(TAG, "Font pointers: g_font_ext_puhui_20_4=%p, BUILTIN_TEXT_FONT=%p", 
           (void*)&g_font_ext_puhui_20_4, (void*)&BUILTIN_TEXT_FONT);
    ESP_LOGI(TAG, "Font initialization completed - using original project fonts");
}
