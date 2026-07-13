/**
 * @file chat_background.c
 * @brief 聊天背景管理模块实现
 */
#include "chat_background.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "chat_bg";

// NVS存储配置
#define NVS_NAMESPACE "chat_bg"
#define NVS_KEY_PATH "bg_path"
#define NVS_KEY_TYPE "bg_type"
#define MAX_PATH_LEN 128

// 图片缓存条目
typedef struct
{
    char path[MAX_PATH_LEN]; // 文件路径（作为缓存键）
    void *img_data;          // 图片数据缓冲区
    size_t img_size;         // 数据大小
    lv_img_dsc_t img_dsc;    // LVGL 图片描述符
    bool is_valid;           // 缓存是否有效
} bg_image_cache_t;

// 模块状态
static char s_bg_path[MAX_PATH_LEN] = {0};
static chat_bg_type_t s_bg_type = CHAT_BG_TYPE_NONE;
static bool s_initialized = false;

// 全局缓存（单例模式，因为同时只有一个背景）
static bg_image_cache_t s_image_cache = {0};

// ============== 路径标准化 ==============

// 挂载点映射表
typedef struct
{
    char drive_letter;        // LVGL 盘符 (如 'S', 'A', 'P')
    const char *mount_point;  // 实际挂载点路径
    const char *strip_prefix; // 需要从虚拟路径中移除的前缀（可选）
} mount_mapping_t;

// 预定义的挂载点映射
static const mount_mapping_t s_mount_mappings[] = {
    {'S', "/sdcard", NULL}, // S:/animation/03.vpg → /sdcard/animation/03.vpg
    {'A', "/assets", NULL}, // A:/icon.png → /assets/icon.png
    {'P', "/sdcard/photo",
     "/photo"},     // P:/photo/03.jpg → /sdcard/photo/03.jpg (移除 /photo)
    {0, NULL, NULL} // 结束标记
};

/**
 * @brief 检测路径是否为 LVGL 虚拟路径
 * @param path 待检测路径
 * @return true=是虚拟路径, false=不是虚拟路径
 * @note 虚拟路径格式: "X:/..." 其中 X 是单个大写字母
 */
static bool is_lvgl_virtual_path(const char *path)
{
    if (!path)
    {
        return false;
    }

    size_t len = strlen(path);
    if (len < 3)
    {
        return false;
    }

    // 检查格式: X:/
    // 第一个字符必须是大写字母 (A-Z)
    // 第二个字符必须是 ':'
    // 第三个字符必须是 '/'
    return (path[0] >= 'A' && path[0] <= 'Z' && path[1] == ':' && path[2] == '/');
}

/**
 * @brief 标准化文件路径（LVGL 虚拟路径 -> 实际文件系统路径）
 * @param lvgl_path LVGL 虚拟路径（如 "P:/photo/03.jpg"）
 * @param out_real_path 输出缓冲区，存储转换后的实际路径
 * @param out_size 输出缓冲区大小
 * @return true=转换成功, false=转换失败
 * @note 如果输入已经是实际路径（不含盘符），则直接复制
 */
static bool normalize_path(const char *lvgl_path, char *out_real_path,
                           size_t out_size)
{
    if (!lvgl_path || !out_real_path || out_size == 0)
    {
        ESP_LOGE(TAG, "Invalid parameters for normalize_path");
        return false;
    }

    // 检查是否为 LVGL 虚拟路径
    if (!is_lvgl_virtual_path(lvgl_path))
    {
        // 不是虚拟路径，直接复制
        size_t len = strlen(lvgl_path);
        if (len >= out_size)
        {
            ESP_LOGE(TAG, "Output buffer too small for path: %s", lvgl_path);
            return false;
        }
        strncpy(out_real_path, lvgl_path, out_size - 1);
        out_real_path[out_size - 1] = '\0';
        return true;
    }

    // 提取盘符
    char drive_letter = lvgl_path[0];

    // 在映射表中查找对应的挂载点
    const mount_mapping_t *mapping = NULL;
    for (int i = 0; s_mount_mappings[i].drive_letter != 0; i++)
    {
        if (s_mount_mappings[i].drive_letter == drive_letter)
        {
            mapping = &s_mount_mappings[i];
            break;
        }
    }

    if (!mapping)
    {
        ESP_LOGE(TAG, "Unknown drive letter: %c", drive_letter);
        return false;
    }

    // 提取虚拟路径中盘符后的部分（跳过 "X:"）
    const char *virtual_subpath = lvgl_path + 2; // 跳过 "X:"

    // 如果映射表中定义了 strip_prefix，需要移除该前缀
    if (mapping->strip_prefix)
    {
        size_t prefix_len = strlen(mapping->strip_prefix);
        // 检查路径是否以该前缀开头
        if (strncmp(virtual_subpath, mapping->strip_prefix, prefix_len) == 0)
        {
            // 跳过前缀部分
            virtual_subpath += prefix_len;
        }
    }

    // 构造新路径：mount_point + 处理后的路径
    size_t mount_len = strlen(mapping->mount_point);
    size_t subpath_len = strlen(virtual_subpath);
    size_t total_len = mount_len + subpath_len;

    if (total_len >= out_size)
    {
        ESP_LOGE(TAG, "Output buffer too small: need %zu, have %zu", total_len + 1,
                 out_size);
        return false;
    }

    // 复制挂载点
    strcpy(out_real_path, mapping->mount_point);
    // 追加子路径
    strcat(out_real_path, virtual_subpath);

    ESP_LOGD(TAG, "Path normalized: %s -> %s", lvgl_path, out_real_path);
    return true;
}

// ============== 内部工具函数 ==============

/**
 * @brief 根据文件扩展名判断背景类型
 */
static chat_bg_type_t detect_type_from_path(const char *path)
{
    if (!path || strlen(path) < 4)
    {
        return CHAT_BG_TYPE_NONE;
    }

    const char *dot = strrchr(path, '.');
    if (!dot)
    {
        return CHAT_BG_TYPE_NONE;
    }

    // 图片类型
    if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0 ||
        strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".bmp") == 0)
    {
        return CHAT_BG_TYPE_IMAGE;
    }

    // VPG动画类型
    if (strcasecmp(dot, ".vpg") == 0 || strcasecmp(dot, ".dat") == 0)
    {
        return CHAT_BG_TYPE_VPG;
    }

    return CHAT_BG_TYPE_NONE;
}

/**
 * @brief 检查文件是否存在
 */
static bool file_exists(const char *path)
{
    if (!path || strlen(path) == 0)
    {
        return false;
    }

    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

/**
 * @brief 保存设置到NVS
 */
static bool save_to_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return false;
    }

    bool success = true;

    // 保存路径
    if (s_bg_type == CHAT_BG_TYPE_NONE || strlen(s_bg_path) == 0)
    {
        err = nvs_erase_key(handle, NVS_KEY_PATH);
        // 忽略 ESP_ERR_NVS_NOT_FOUND 错误
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "NVS erase path failed: %s", esp_err_to_name(err));
            success = false;
        }
    }
    else
    {
        err = nvs_set_str(handle, NVS_KEY_PATH, s_bg_path);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS set path failed: %s", esp_err_to_name(err));
            success = false;
        }
    }

    // 保存类型
    err = nvs_set_u8(handle, NVS_KEY_TYPE, (uint8_t)s_bg_type);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS set type failed: %s", esp_err_to_name(err));
        success = false;
    }

    // 提交更改
    if (success)
    {
        err = nvs_commit(handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
            success = false;
        }
    }

    nvs_close(handle);
    return success;
}

/**
 * @brief 从NVS加载设置
 */
static void load_from_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "NVS open failed (first run?): %s", esp_err_to_name(err));
        return;
    }

    // 读取类型
    uint8_t type = 0;
    err = nvs_get_u8(handle, NVS_KEY_TYPE, &type);
    if (err == ESP_OK && type <= CHAT_BG_TYPE_VPG)
    {
        s_bg_type = (chat_bg_type_t)type;
    }

    // 读取路径
    if (s_bg_type != CHAT_BG_TYPE_NONE)
    {
        size_t len = sizeof(s_bg_path);
        err = nvs_get_str(handle, NVS_KEY_PATH, s_bg_path, &len);
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "NVS get path failed: %s", esp_err_to_name(err));
            s_bg_path[0] = '\0';
            s_bg_type = CHAT_BG_TYPE_NONE;
        }
    }

    nvs_close(handle);

    ESP_LOGI(TAG, "Loaded: type=%d, path=%s", s_bg_type, s_bg_path);
}

// ============== 公开 API ==============

void chat_background_init(void)
{
    if (s_initialized)
    {
        return;
    }

    // 清空状态
    memset(s_bg_path, 0, sizeof(s_bg_path));
    s_bg_type = CHAT_BG_TYPE_NONE;

    // 从NVS加载设置
    load_from_nvs();

    // 验证加载的背景是否有效
    if (s_bg_type != CHAT_BG_TYPE_NONE && !file_exists(s_bg_path))
    {
        ESP_LOGW(TAG, "Background file not found, clearing: %s", s_bg_path);
        s_bg_path[0] = '\0';
        s_bg_type = CHAT_BG_TYPE_NONE;
        save_to_nvs();
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Init complete: type=%d, path=%s", s_bg_type,
             s_bg_type != CHAT_BG_TYPE_NONE ? s_bg_path : "(none)");
}

bool chat_background_set(const char *path)
{
    if (!path || strlen(path) == 0)
    {
        ESP_LOGE(TAG, "Invalid path");
        return false;
    }

    if (strlen(path) >= MAX_PATH_LEN)
    {
        ESP_LOGE(TAG, "Path too long: %zu", strlen(path));
        return false;
    }

    // 检查文件是否存在
    if (!file_exists(path))
    {
        ESP_LOGE(TAG, "File not found: %s", path);
        return false;
    }

    // 检测文件类型
    chat_bg_type_t type = detect_type_from_path(path);
    if (type == CHAT_BG_TYPE_NONE)
    {
        ESP_LOGE(TAG, "Unsupported file type: %s", path);
        return false;
    }

    // 更新状态
    strncpy(s_bg_path, path, MAX_PATH_LEN - 1);
    s_bg_path[MAX_PATH_LEN - 1] = '\0';
    s_bg_type = type;

    // 保存到NVS
    if (!save_to_nvs())
    {
        ESP_LOGE(TAG, "Failed to save to NVS");
        return false;
    }

    ESP_LOGI(TAG, "Background set: type=%d, path=%s", s_bg_type, s_bg_path);
    return true;
}

const char *chat_background_get_path(void)
{
    if (s_bg_type == CHAT_BG_TYPE_NONE || strlen(s_bg_path) == 0)
    {
        return NULL;
    }
    return s_bg_path;
}

chat_bg_type_t chat_background_get_type(void) { return s_bg_type; }

void chat_background_clear(void)
{
    s_bg_path[0] = '\0';
    s_bg_type = CHAT_BG_TYPE_NONE;

    if (!save_to_nvs())
    {
        ESP_LOGE(TAG, "Failed to clear NVS");
    }

    ESP_LOGI(TAG, "Background cleared");
}

bool chat_background_validate(void)
{
    if (s_bg_type == CHAT_BG_TYPE_NONE)
    {
        return true; // 无背景设置，视为有效
    }

    if (!file_exists(s_bg_path))
    {
        ESP_LOGW(TAG, "Background file invalid, clearing: %s", s_bg_path);
        chat_background_clear();
        return false;
    }

    return true;
}

bool chat_background_is_current(const char *path)
{
    if (!path || s_bg_type == CHAT_BG_TYPE_NONE || strlen(s_bg_path) == 0)
    {
        return false;
    }

    return (strcmp(s_bg_path, path) == 0);
}

// ============== 图片缓存管理 ==============

bool chat_background_load_to_cache(const char *path, void **out_img_dsc)
{
    if (!path || !out_img_dsc)
    {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // 检查缓存是否命中
    if (s_image_cache.is_valid && strcmp(s_image_cache.path, path) == 0 &&
        s_image_cache.img_data != NULL)
    {
        ESP_LOGI(TAG, "Cache hit: %s", path);
        *out_img_dsc = (void *)&s_image_cache.img_dsc;
        return true;
    }

    // 缓存未命中，需要加载文件
    ESP_LOGI(TAG, "Cache miss, loading: %s", path);

    if (s_image_cache.is_valid && s_image_cache.img_data != NULL) 
    {
    lv_image_cache_drop((const void *)&s_image_cache.img_dsc);
    }

    // 清除旧缓存
    if (s_image_cache.img_data != NULL)
    {
        free(s_image_cache.img_data);
        s_image_cache.img_data = NULL;
    }
    memset(&s_image_cache, 0, sizeof(s_image_cache));

    // 标准化路径（LVGL 虚拟路径 -> 实际文件系统路径）
    char real_path[MAX_PATH_LEN];
    if (!normalize_path(path, real_path, sizeof(real_path)))
    {
        ESP_LOGE(TAG, "Failed to normalize path: %s", path);
        return false;
    }

    // 打开文件（使用标准化后的实际路径）
    FILE *fp = fopen(real_path, "rb");
    if (!fp)
    {
        ESP_LOGE(TAG, "Failed to open file: %s (normalized from: %s)", real_path,
                 path);
        return false;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0)
    {
        ESP_LOGE(TAG, "Invalid file size: %ld", file_size);
        fclose(fp);
        return false;
    }

    // 分配内存缓冲区
    void *buffer = malloc(file_size);
    if (!buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate %ld bytes", file_size);
        fclose(fp);
        return false;
    }

    // 读取文件数据
    size_t bytes_read = fread(buffer, 1, file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size)
    {
        ESP_LOGE(TAG, "Read error: expected %ld, got %zu", file_size, bytes_read);
        free(buffer);
        return false;
    }

    // 填充缓存结构（存储原始 LVGL 路径作为缓存键）
    strncpy(s_image_cache.path, path, MAX_PATH_LEN - 1);
    s_image_cache.path[MAX_PATH_LEN - 1] = '\0';
    s_image_cache.img_data = buffer;
    s_image_cache.img_size = file_size;
    s_image_cache.is_valid = true;

    // 填充 LVGL 图片描述符
    memset(&s_image_cache.img_dsc, 0, sizeof(lv_img_dsc_t));
    s_image_cache.img_dsc.data = (const uint8_t *)buffer;
    s_image_cache.img_dsc.data_size = file_size;
    // 注意：header 字段（宽度、高度、格式）由 LVGL 在解码时自动填充

    *out_img_dsc = (void *)&s_image_cache.img_dsc;
    ESP_LOGI(TAG, "Loaded to cache: %s (%zu bytes)", path, file_size);

    return true;
}

void chat_background_clear_cache(void)
{
    if (s_image_cache.img_data != NULL)
    {
         if (s_image_cache.is_valid)
        {
            lv_image_cache_drop((const void *)&s_image_cache.img_dsc);
        }
        free(s_image_cache.img_data);
        s_image_cache.img_data = NULL;
        ESP_LOGI(TAG, "Cache cleared: %s", s_image_cache.path);
    }

    // 重置缓存状态
    memset(&s_image_cache, 0, sizeof(s_image_cache));
}

bool chat_background_is_cached(const char *path)
{
    if (!path)
    {
        return false;
    }

    return (s_image_cache.is_valid && s_image_cache.img_data != NULL &&
            strcmp(s_image_cache.path, path) == 0);
}
