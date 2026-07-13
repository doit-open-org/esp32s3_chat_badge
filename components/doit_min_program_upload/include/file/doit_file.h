#ifndef __DOIT_FILE_H__
#define __DOIT_FILE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "file_common.h"

    // 文件类型枚举
    typedef enum
    {
        MATERIAL_TYPE_VPG = 0, // animal目录
        MATERIAL_TYPE_IMG = 1, // photo目录
        MATERIAL_TYPE_COUNT
    } material_type_t;
    void doit_file_system_init(void);
    void doit_file_page_init(lv_obj_t *display_obj, uint16_t width,
                             uint16_t height);

    void doit_file_init(lv_obj_t *psd_obj_, uint16_t width, uint16_t height);
    void doit_file_decode();
    void doit_file_handle_startup_display(void);
    void doit_file_mark_downloaded(void);
    void doit_file_set_show_upload_ui_callback(void (*callback)(void));
    void doit_file_set_refresh_display_callback(
        void (*callback)(void));          // 新增：设置刷新显示回调
    void doit_file_refresh_display(void); // 手动触发刷新显示
    char *get_show_dir(void);
    char *detect_file_type_from_fs(void);
    // 新的接口函数
    void doit_file_set_type(material_type_t type);
    material_type_t doit_file_get_type(void);
    uint32_t doit_file_get_count(material_type_t type);
    uint32_t doit_file_get_current_index(void);
    void doit_file_switch_next(void);      // 切换到下一个素材
    void doit_file_switch_prev(void);      // 切换到上一个素材
    void doit_file_rescan_materials(void); // 重新扫描文件（慢，仅启动时使用）
    bool doit_file_add_downloaded(const char *filepath,
                                  material_type_t type); // 下载后快速追加（推荐）
    void doit_file_switch_to_latest(void);               // 切换到最新文件（当前类型）
    void doit_file_switch_to_latest_by_type(
        material_type_t type); // 切换指定类型到最新文件
    // 删除功能
    doit_file_result_t doit_file_delete_current(void); // 删除当前文件
    bool doit_file_is_last_file(void);                 // 检查是否为最后一个文件
    const char *doit_file_get_next_after_delete(void); // 获取删除后应显示的文件
    bool doit_file_has_images(void);                   // 检查是否有图片文件
    bool doit_file_has_vpg(void);                      // 检查是否有VPG动画文件
    char *doit_file_get_current_path(void);            // 获取当前文件路径
    char *doit_file_get_by_offset(int offset);         // 获取指定偏移的文件路径（需要free）
    uint32_t doit_file_get_next_download_index(
        material_type_t type); // 获取下一个可用的下载索引

    // 线程安全的刷新通知接口（FreeRTOS事件组）
    void doit_file_notify_refresh_needed(void); // 通知需要刷新（BLE任务调用）
    bool doit_file_check_refresh_needed(void);  // 检查并清除刷新事件（LVGL任务调用）

#ifdef __cplusplus
}
#endif

#endif // __DOIT_FILE_H__