/**
 * @file doit_file.c
 * @brief 素材文件管理模块
 */
#include "doit_file.h"
#include "chat_background.h"
#include "doit_decode.h"
#include "doit_ui.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "img_decode.h"
#include "vpg_decode.h"
#include <dirent.h>
#include <errno.h>
#include <esp_log.h>
#include <sys/stat.h>
#include <unistd.h>

extern bool zw_sdcard_is_usb_mode(void);
static const char *TAG = "doit_file";

// 事件组位定义
#define REFRESH_EVENT_BIT (1 << 0) // 需要刷新显示

// 事件组句柄
static EventGroupHandle_t s_refresh_event_group = NULL;

// 回调函数
static void (*s_show_upload_ui_callback)(void) = NULL;
static void (*s_refresh_display_callback)(void) = NULL;

// 文件信息结构
typedef struct
{
    uint32_t index;
    char filepath[256];
    char extension[8];
} material_file_t;

// 分组管理结构
typedef struct
{
    material_file_t *files;
    uint32_t count;
    uint32_t capacity;
    uint32_t current_index;
} material_group_t;

// 目录配置
typedef struct
{
    const char *dir_path;
    const char **extensions;
} dir_config_t;

// 支持的扩展名
static const char *vpg_extensions[] = {".dat", ".vpg", NULL};
static const char *img_extensions[] = {".jpg", ".jpeg", ".png", ".bmp", NULL};

// 目录配置
static const dir_config_t dir_configs[MATERIAL_TYPE_COUNT] = {
    {"/sdcard/animal", vpg_extensions}, {"/sdcard/photo", img_extensions}};

// 全局状态
static material_group_t g_groups[MATERIAL_TYPE_COUNT] = {0};
static material_type_t g_current_type = MATERIAL_TYPE_VPG;
static bool s_initialized = false;

// ============== 内部工具函数 ==============

static bool is_ext_supported(const char *ext, material_type_t type)
{
    for (const char **p = dir_configs[type].extensions; *p; p++)
    {
        if (strcasecmp(ext, *p) == 0)
            return true;
    }
    return false;
}

static bool parse_filename(const char *name, uint32_t *idx, char *ext)
{
    const char *dot = strrchr(name, '.');
    if (!dot)
        return false;

    strncpy(ext, dot, 7);
    ext[7] = '\0';
    for (char *p = ext; *p; p++)
    {
        if (*p >= 'A' && *p <= 'Z')
            *p += 32;
    }

    char num[64];
    size_t len = dot - name;
    if (len >= sizeof(num))
        return false;
    strncpy(num, name, len);
    num[len] = '\0';

    char *end;
    long val = strtol(num, &end, 10);
    if (end == num || val <= 0)
        return false;

    *idx = (uint32_t)val;
    return true;
}

static void sort_files(material_group_t *g)
{
    for (uint32_t i = 0; i < g->count; i++)
    {
        for (uint32_t j = i + 1; j < g->count; j++)
        {
            if (g->files[i].index > g->files[j].index)
            {
                material_file_t tmp = g->files[i];
                g->files[i] = g->files[j];
                g->files[j] = tmp;
            }
        }
    }
}

static void normalize_indices(material_type_t type)
{
    material_group_t *g = &g_groups[type];
    const char *dir = dir_configs[type].dir_path;

    for (uint32_t i = 0; i < g->count; i++)
    {
        uint32_t new_idx = i + 1;
        if (g->files[i].index != new_idx)
        {
            char new_path[512];
            snprintf(new_path, sizeof(new_path), "%s/%02lu%s", dir,
                     (unsigned long)new_idx, g->files[i].extension);

            if (rename(g->files[i].filepath, new_path) == 0)
            {
                g->files[i].index = new_idx;
                strncpy(g->files[i].filepath, new_path,
                        sizeof(g->files[i].filepath) - 1);
            }
        }
    }
}

static bool validate_file(const char *path, const char *ext)
{
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size < 32)
        return false;

    FILE *f = fopen(path, "rb");
    if (!f)
        return false;

    uint8_t hdr[4] = {0};
    size_t n = fread(hdr, 1, 4, f);
    fclose(f);

    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
    {
        return (n >= 3 && hdr[0] == 0xFF && hdr[1] == 0xD8 && hdr[2] == 0xFF);
    }
    if (strcasecmp(ext, ".vpg") == 0 || strcasecmp(ext, ".dat") == 0)
    {
        uint32_t magic = *(uint32_t *)hdr;
        return (magic == 0xAABBCCDD);
    }
    if (strcasecmp(ext, ".png") == 0)
    {
        return (n >= 4 && hdr[0] == 0x89 && hdr[1] == 'P' && hdr[2] == 'N' &&
                hdr[3] == 'G');
    }
    return true; // 其他格式默认有效
}

static void scan_directory(material_type_t type)
{
    // USB模式下不执行扫描
    if (zw_sdcard_is_usb_mode())
    {
        ESP_LOGW(TAG, "scan_directory blocked: USB mode active");
        return;
    }
    material_group_t *g = &g_groups[type];
    const char *dir_path = dir_configs[type].dir_path;

    // 保存当前索引，扫描后尝试恢复
    uint32_t saved_index = g->current_index;

    // 清理旧数据
    if (g->files)
        free(g->files);
    g->files = NULL;
    g->count = 0;
    g->capacity = 0;
    // 注意：不重置 current_index，后面会处理

    // 确保目录存在
    struct stat st;
    if (stat(dir_path, &st) != 0)
    {
        mkdir(dir_path, 0755);
    }

    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        // 目录打开失败，重置索引
        g->current_index = 0;
        return;
    }

    g->capacity = 16;
    g->files = malloc(g->capacity * sizeof(material_file_t));
    if (!g->files)
    {
        closedir(dir);
        g->current_index = 0;
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_type != DT_REG)
            continue;

        uint32_t idx;
        char ext[8];
        if (!parse_filename(ent->d_name, &idx, ext))
            continue;
        if (!is_ext_supported(ext, type))
            continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%02lu%s", dir_path, (unsigned long)idx,
                 ext);

        if (!validate_file(path, ext))
        {
            unlink(path);
            continue;
        }

        // 扩容
        if (g->count >= g->capacity)
        {
            g->capacity *= 2;
            g->files = realloc(g->files, g->capacity * sizeof(material_file_t));
            if (!g->files)
                break;
        }

        material_file_t *f = &g->files[g->count++];
        f->index = idx;
        strncpy(f->filepath, path, sizeof(f->filepath) - 1);
        strncpy(f->extension, ext, sizeof(f->extension) - 1);
    }
    closedir(dir);

    sort_files(g);
    normalize_indices(type);

    // 恢复之前保存的索引，确保不越界
    if (g->count == 0)
    {
        g->current_index = 0;
    }
    else if (saved_index < g->count)
    {
        g->current_index = saved_index;
    }
    else
    {
        // 之前的索引超出范围，设置为最后一个
        g->current_index = g->count - 1;
    }

    ESP_LOGI(TAG,
             "scan_directory(%d): count=%lu, saved_index=%lu, current_index=%lu",
             type, (unsigned long)g->count, (unsigned long)saved_index,
             (unsigned long)g->current_index);
}

// ============== 公开 API ==============

void doit_file_system_init(void)
{
    if (s_initialized)
        return;

    struct stat st;
    if (stat("/sdcard", &st) != 0)
    {
        ESP_LOGE(TAG, "SD card not mounted");
        return;
    }

    // 创建事件组用于刷新通知
    if (s_refresh_event_group == NULL)
    {
        s_refresh_event_group = xEventGroupCreate();
        if (s_refresh_event_group == NULL)
        {
            ESP_LOGE(TAG, "Failed to create refresh event group");
        }
        else
        {
            ESP_LOGI(TAG, "Refresh event group created");
        }
    }

    scan_directory(MATERIAL_TYPE_VPG);
    scan_directory(MATERIAL_TYPE_IMG);
    doit_decode_init();

    s_initialized = true;
    ESP_LOGI(TAG, "Init: VPG=%lu, IMG=%lu",
             (unsigned long)g_groups[MATERIAL_TYPE_VPG].count,
             (unsigned long)g_groups[MATERIAL_TYPE_IMG].count);
}

void doit_file_page_init(lv_obj_t *obj, uint16_t w, uint16_t h)
{
    if (!s_initialized)
        doit_file_system_init();
    if (obj)
        doit_ui_init(obj, w, h);
}

void doit_file_init(lv_obj_t *obj, uint16_t w, uint16_t h)
{
    doit_file_system_init();
    doit_file_page_init(obj, w, h);
}

char *get_show_dir(void)
{
    static char path[128];
    material_group_t *g = &g_groups[g_current_type];

    if (g->count > 0 && g->current_index < g->count)
    {
        strncpy(path, g->files[g->current_index].filepath, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        return path;
    }
    return "/sdcard/photo/01.jpg";
}

char *detect_file_type_from_fs(void)
{
    char *path = get_show_dir();
    uint8_t hdr[12] = {0};

    FILE *f = fopen(path, "rb");
    if (!f)
        return "unknown";
    fread(hdr, 1, sizeof(hdr), f);
    fclose(f);

    if (*(uint32_t *)hdr == 0xAABBCCDD)
        return "vpg";
    if (hdr[0] == 0xFF && hdr[1] == 0xD8)
        return "jpg";
    if (memcmp(hdr, "\x89PNG", 4) == 0)
        return "png";
    if (hdr[0] == 'B' && hdr[1] == 'M')
        return "bmp";

    return "unknown";
}

void doit_file_decode(void)
{
    if (zw_sdcard_is_usb_mode())
    {
        ESP_LOGW(TAG, "doit_file_decode blocked: USB mode active");
        return;
    }
    char *path = get_show_dir();
    if (!path)
    {
        ESP_LOGW(TAG, "doit_file_decode: no file path available");
        return;
    }

    material_type_t type = doit_file_get_type();
    ESP_LOGI(TAG, "doit_file_decode: type=%d, path=%s", type, path);

    if (type == MATERIAL_TYPE_VPG)
    {
        // VPG动画：调用vpg_decode播放器
        doit_vpg_player_start(path);
    }
    else if (type == MATERIAL_TYPE_IMG)
    {
        // 静态图片：调用img_decode
        doit_img_decode(path);
    }
}

void doit_file_set_type(material_type_t type)
{
    if (type < MATERIAL_TYPE_COUNT)
    {
        g_current_type = type;
    }
}

material_type_t doit_file_get_type(void) { return g_current_type; }

uint32_t doit_file_get_count(material_type_t type)
{
    return (type < MATERIAL_TYPE_COUNT) ? g_groups[type].count : 0;
}

uint32_t doit_file_get_current_index(void)
{
    material_group_t *g = &g_groups[g_current_type];
    return (g->count > 0) ? g->files[g->current_index].index : 1;
}

void doit_file_switch_next(void)
{
    material_group_t *g = &g_groups[g_current_type];
    if (g->count == 0)
        return;

    g->current_index = (g->current_index + 1) % g->count;
    if (s_refresh_display_callback)
        s_refresh_display_callback();
}

void doit_file_switch_prev(void)
{
    material_group_t *g = &g_groups[g_current_type];
    if (g->count == 0)
        return;

    g->current_index =
        (g->current_index == 0) ? (g->count - 1) : (g->current_index - 1);
    if (s_refresh_display_callback)
        s_refresh_display_callback();
}

void doit_file_handle_startup_display(void)
{
    struct stat st;
    if (stat(get_show_dir(), &st) == 0)
    {
        doit_file_decode();
    }
    else if (s_show_upload_ui_callback)
    {
        lv_obj_t *obj = doit_ui_get_show_lv_obj();
        if (obj)
        {
            lvgl_port_lock(-1);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lvgl_port_unlock();
        }
        s_show_upload_ui_callback();
    }
}

void doit_file_mark_downloaded(void) { /* 保留接口 */ }

void doit_file_set_show_upload_ui_callback(void (*cb)(void))
{
    s_show_upload_ui_callback = cb;
}
void doit_file_set_refresh_display_callback(void (*cb)(void))
{
    s_refresh_display_callback = cb;
}

void doit_file_rescan_materials(void)
{
    scan_directory(MATERIAL_TYPE_VPG);
    scan_directory(MATERIAL_TYPE_IMG);
}

/**
 * @brief 下载完成后直接追加文件到内存索引（不重新扫描目录）
 * @param filepath 完整文件路径，如 "/sdcard/photo/05.jpg"
 * @param type 素材类型
 * @return true 成功，false 失败
 */
bool doit_file_add_downloaded(const char *filepath, material_type_t type)
{
    if (!filepath || type >= MATERIAL_TYPE_COUNT)
        return false;

    material_group_t *g = &g_groups[type];

    // 解析文件名获取索引和扩展名
    const char *filename = strrchr(filepath, '/');
    if (!filename)
        return false;
    filename++; // 跳过 '/'

    uint32_t idx;
    char ext[8];
    if (!parse_filename(filename, &idx, ext))
        return false;

    // 检查是否需要扩容
    if (g->count >= g->capacity)
    {
        uint32_t new_cap = (g->capacity == 0) ? 16 : g->capacity * 2;
        material_file_t *new_files =
            realloc(g->files, new_cap * sizeof(material_file_t));
        if (!new_files)
            return false;
        g->files = new_files;
        g->capacity = new_cap;
    }

    // 追加新文件
    material_file_t *f = &g->files[g->count];
    f->index = idx;
    strncpy(f->filepath, filepath, sizeof(f->filepath) - 1);
    f->filepath[sizeof(f->filepath) - 1] = '\0';
    strncpy(f->extension, ext, sizeof(f->extension) - 1);
    f->extension[sizeof(f->extension) - 1] = '\0';
    g->count++;

    // 设置当前索引为新文件（最后一个）
    g->current_index = g->count - 1;

    ESP_LOGI(TAG, "add_downloaded: %s, type=%d, count=%lu, current=%lu", filepath,
             type, (unsigned long)g->count, (unsigned long)g->current_index);

    return true;
}

void doit_file_switch_to_latest(void)
{
    material_group_t *g = &g_groups[g_current_type];
    if (g->count == 0)
        return;

    uint32_t max_i = 0;
    for (uint32_t i = 1; i < g->count; i++)
    {
        if (g->files[i].index > g->files[max_i].index)
            max_i = i;
    }
    g->current_index = max_i;
}

/**
 * @brief 通知需要刷新显示（线程安全）
 * @note 从BLE任务调用，设置事件位通知LVGL任务刷新
 */
void doit_file_notify_refresh_needed(void)
{
    if (s_refresh_event_group != NULL)
    {
        xEventGroupSetBits(s_refresh_event_group, REFRESH_EVENT_BIT);
        ESP_LOGI(TAG, "Refresh event set");
    }
}

/**
 * @brief 检查并清除刷新事件（线程安全）
 * @return true 需要刷新，false 不需要刷新
 * @note 从LVGL任务调用，检查是否有待处理的刷新请求
 */
bool doit_file_check_refresh_needed(void)
{
    if (s_refresh_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(s_refresh_event_group);
    if (bits & REFRESH_EVENT_BIT)
    {
        // 清除事件位
        xEventGroupClearBits(s_refresh_event_group, REFRESH_EVENT_BIT);
        ESP_LOGI(TAG, "Refresh event detected and cleared");
        return true;
    }

    return false;
}

void doit_file_switch_to_latest_by_type(material_type_t type)
{
    if (type >= MATERIAL_TYPE_COUNT)
        return;

    material_group_t *g = &g_groups[type];
    if (g->count == 0)
        return;

    uint32_t max_i = 0;
    for (uint32_t i = 1; i < g->count; i++)
    {
        if (g->files[i].index > g->files[max_i].index)
            max_i = i;
    }
    g->current_index = max_i;

    // // 如果当前类型就是指定类型，触发刷新回调
    // if (g_current_type == type && s_refresh_display_callback)
    // {
    //     s_refresh_display_callback();
    // }
}

bool doit_file_is_last_file(void)
{
    return g_groups[g_current_type].count <= 1;
}

bool doit_file_has_images(void)
{
    return (g_current_type == MATERIAL_TYPE_IMG &&
            g_groups[MATERIAL_TYPE_IMG].count > 0);
}

bool doit_file_has_vpg(void) { return g_groups[MATERIAL_TYPE_VPG].count > 0; }

doit_file_result_t doit_file_delete_current(void)
{
    // USB模式下不执行删除
    if (zw_sdcard_is_usb_mode())
    {
        ESP_LOGW(TAG, "doit_file_delete_current blocked: USB mode active");
        return (doit_file_result_t){.err_code = CL_OPERT_FAIL, .path = NULL};
    }
    material_group_t *g = &g_groups[g_current_type];
    if (g->count == 0)
    {
        return (doit_file_result_t){.err_code = CL_OPERT_FAIL, .path = NULL};
    }

    uint32_t idx = g->current_index;

    if (chat_background_is_current(g->files[idx].filepath))
    {
        ESP_LOGI(TAG, "Deleting current chat background, clearing setting");
        chat_background_clear();
    }

    // 删除文件
    if (remove(g->files[idx].filepath) != 0)
    {
        return (doit_file_result_t){.err_code = CL_OPERT_FAIL, .path = NULL};
    }

    // 移除数组元素
    for (uint32_t i = idx; i < g->count - 1; i++)
    {
        g->files[i] = g->files[i + 1];
    }
    g->count--;

    // 重新规范化索引
    normalize_indices(g_current_type);

    // 调整当前索引
    if (g->count == 0)
    {
        g->current_index = 0;
    }
    else if (idx >= g->count)
    {
        g->current_index = g->count - 1;
    }

    return (doit_file_result_t){.err_code = CL_OPRET_SUCCESS, .path = NULL};
}

const char *doit_file_get_next_after_delete(void)
{
    material_group_t *g = &g_groups[MATERIAL_TYPE_IMG];
    if (g->count > 1)
    {
        uint32_t next = (g->current_index + 1) % g->count;
        return g->files[next].filepath;
    }
    return NULL;
}

char *doit_file_get_by_offset(int offset)
{
    material_group_t *g = &g_groups[g_current_type];
    if (g->count == 0)
        return NULL;

    int idx = ((int)g->current_index + offset) % (int)g->count;
    if (idx < 0)
        idx += g->count;

    g->current_index = idx;
    return strdup(g->files[idx].filepath);
}

char *doit_file_get_current_path(void) { return get_show_dir(); }

uint32_t doit_file_get_next_download_index(material_type_t type)
{
    if (type >= MATERIAL_TYPE_COUNT)
        return 1;

    material_group_t *g = &g_groups[type];
    if (g->count == 0)
        return 1;

    uint32_t max = 0;
    for (uint32_t i = 0; i < g->count; i++)
    {
        if (g->files[i].index > max)
            max = g->files[i].index;
    }
    return max + 1;
}
