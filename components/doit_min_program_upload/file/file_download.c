#include "doit_file.h"
#include "doit_ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"

#include "doit_nvs.h"
#include "vpg_decode.h"

static const char *TAG = "file_download";

/*==========================宏定义===================================*/
#define HTTP_DOWNLOAD_CHUNK_BUFFER (16 * 1024)       // 16KB，流式下载足够且更省内存
#define HTTP_DOWNLOAD_MAX_SIZE (100 * 1024 * 1024)   // 最大下载文件100MB
#define FS_WRITE_CHUNK (128 * 1024)              // 文件写入块大小
/*===================================================================*/

static int64_t s_content_len = 0; // 文件总长度（未知时可能为-1）
static bool s_download_busy = false;

typedef struct
{
    uint8_t cur_sta;        // 当前状态,0:初始化,1:正在下载，2：下载完成
    FILE *temp_file;        // 临时文件句柄（直接写SD卡）
    char *temp_path;        // 临时文件路径
    char *final_path;       // 最终完整路径
    uint32_t total_written; // 统计：已经写盘的总字节
    uint32_t file_total;    // 服务器发过来的文件总大小
} http_save_file_t;

#define DL_STA_NO_SPACE 3

/* =============静态函数声明===================*/
static bool is_http_file_content_length_overflow(const char *url);
static doit_file_result_t to_download(const char *url, const char *dir_name);
static char *get_file_type_in_url(const char *url);
static doit_file_result_t http_download_chunk(const char *file_url, const char *dir_name);
static uint8_t calc_download_percent(uint32_t written, uint32_t total);
static bool is_supported_download_dir(const char *dir_name);

/* HTTP事件回调 - 直接流式写入SD卡临时文件，避免PSRAM内存限制 */
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static uint32_t last_update = 0;
    http_save_file_t *store = (http_save_file_t *)evt->user_data;
    if (store == NULL)
        return ESP_FAIL;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        // 提前创建临时文件，避免服务端无 Content-Length 时无法写入
        if (!store->temp_file && store->temp_path)
        {
            store->temp_file = fopen(store->temp_path, "wb");
            if (!store->temp_file)
            {
                ESP_LOGE(TAG, "无法创建临时文件: %s, errno=%d", store->temp_path, errno);
                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "临时文件已创建: %s", store->temp_path);
        }
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (evt->header_key && evt->header_value &&
            strcasecmp(evt->header_key, "Content-Length") == 0)
        {
            int64_t content_length = atoll(evt->header_value);
            if (content_length > 0)
            {
                if (content_length > HTTP_DOWNLOAD_MAX_SIZE)
                {
                    ESP_LOGE(TAG, "文件大小超限: %s > %d",
                             evt->header_value, HTTP_DOWNLOAD_MAX_SIZE);
                    store->cur_sta = DL_STA_NO_SPACE;
                    download_progress_fail(UI_FAIL_NO_SPACE);
                    return ESP_FAIL;
                }
                store->file_total = (uint32_t)content_length;

                uint64_t fs_total_bytes = 0;
                uint64_t fs_free_bytes = 0;
                if (esp_vfs_fat_info("/sdcard", &fs_total_bytes, &fs_free_bytes) == ESP_OK)
                {
                    if ((uint64_t)content_length > fs_free_bytes)
                    {
                        ESP_LOGE(TAG, "SD卡容量不足: need=%u bytes, free=%u bytes, total=%u bytes",
                                 (unsigned int)content_length, (unsigned int)fs_free_bytes,
                                 (unsigned int)fs_total_bytes);
                        store->cur_sta = DL_STA_NO_SPACE;
                        download_progress_fail(UI_FAIL_NO_SPACE);
                        return ESP_FAIL;
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "无法获取SD剩余容量，继续下载");
                }
            }
            last_update = 0;

            ESP_LOGI(TAG, "文件大小=%s, 使用SD卡临时文件方式下载", evt->header_value);

            // 兜底：若尚未打开，则在拿到头时打开
            if (!store->temp_file && store->temp_path)
            {
                store->temp_file = fopen(store->temp_path, "wb");
                if (!store->temp_file)
                {
                    ESP_LOGE(TAG, "无法创建临时文件: %s, errno=%d", store->temp_path, errno);
                }
                else
                {
                    ESP_LOGI(TAG, "临时文件已创建: %s", store->temp_path);
                }
            }
        }
        break;

    case HTTP_EVENT_ON_DATA:
        if (!store->temp_file)
        {
            ESP_LOGE(TAG, "临时文件未打开, abort");
            download_progress_fail(UI_FAIL_WRITE_ERROR);
            return ESP_FAIL;
        }

        store->cur_sta = 1; // 正在下载

        // 直接写入SD卡临时文件
        size_t written = fwrite(evt->data, 1, evt->data_len, store->temp_file);
        if (written != evt->data_len)
        {
            int write_errno = errno;
            ESP_LOGE(TAG, "写入临时文件失败: expected %d, written %d", evt->data_len, written);
            if (write_errno == ENOSPC)
            {
                ESP_LOGE(TAG, "SD卡容量已满/不足 (errno=%d)", write_errno);
                store->cur_sta = DL_STA_NO_SPACE;
                download_progress_fail(UI_FAIL_NO_SPACE);
            }
            else
            {
                download_progress_fail(UI_FAIL_WRITE_ERROR);
            }
            return ESP_FAIL;
        }
        store->total_written += written;

        // 无 Content-Length 或服务端不可信时，仍执行总量上限保护
        if (store->total_written > HTTP_DOWNLOAD_MAX_SIZE)
        {
            ESP_LOGE(TAG, "下载数据超限: %lu > %d",
                     (unsigned long)store->total_written, HTTP_DOWNLOAD_MAX_SIZE);
            download_progress_fail(UI_FAIL_NO_SPACE);
            return ESP_FAIL;
        }

        // 更新下载进度
        if (store->file_total > 0)
        {
            uint8_t percent = calc_download_percent(store->total_written, store->file_total);
            if (last_update != percent)
            {
                download_progress_update(percent);
                last_update = percent;
            }
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        {
        int status_code = esp_http_client_get_status_code(evt->client);
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH [addr=%p, written=%lu, total=%lu, sta=%d]",
                 store, (unsigned long)store->total_written, (unsigned long)store->file_total, store->cur_sta);

        // 关闭临时文件
        if (store->temp_file)
        {
            fflush(store->temp_file);
            fclose(store->temp_file);
            store->temp_file = NULL;
        }

        // 验证状态码、完整性并移动到最终路径
        if (status_code != 200)
        {
            ESP_LOGE(TAG, "HTTP状态码异常: %d", status_code);
            if (store->temp_path)
            {
                unlink(store->temp_path);
            }
            download_progress_fail(UI_FAIL_HTTP_ERROR);
            break;
        }

        // 验证下载完整性并移动到最终路径
        // 有 Content-Length: 必须完全匹配
        // 无 Content-Length: 只要收到了数据即认为下载成功
        bool is_complete = false;
        if (store->file_total > 0)
        {
            is_complete = (store->total_written == store->file_total);
        }
        else
        {
            is_complete = (store->total_written > 0);
        }

        if (is_complete)
        {
            ESP_LOGI(TAG, "下载完成，开始重命名文件: %s -> %s", store->temp_path, store->final_path);

            // 重命名临时文件到最终路径（同目录下rename很快）
            if (rename(store->temp_path, store->final_path) == 0)
            {
                store->cur_sta = 2; // 仅重命名成功才算下载完成
                ESP_LOGI(TAG, "文件重命名成功，总计 %lu 字节 (%lu KB)",
                         (unsigned long)store->total_written, (unsigned long)store->total_written / 1024);
                download_progress_done();
            }
            else
            {
                ESP_LOGE(TAG, "文件重命名失败, errno=%d", errno);
                unlink(store->temp_path); // 删除临时文件
                download_progress_fail(UI_FAIL_WRITE_ERROR);
            }
        }
        else
        {
            if (store->file_total > 0)
            {
                uint8_t percent = calc_download_percent(store->total_written, store->file_total);
                ESP_LOGE(TAG, "下载不完整: expected %lu, got %lu (%.1f%%)",
                         (unsigned long)store->file_total, (unsigned long)store->total_written,
                         (float)percent);
            }
            else
            {
                ESP_LOGE(TAG, "下载失败: 未获取到 Content-Length 且未收到有效数据");
            }
            // 删除临时文件并显示失败UI
            if (store->temp_path)
            {
                unlink(store->temp_path);
            }
            if (store->cur_sta == DL_STA_NO_SPACE)
            {
                download_progress_fail(UI_FAIL_NO_SPACE);
            }
            else
            {
                download_progress_fail(UI_FAIL_TIMEOUT);
            }
        }
        break;
        }

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED [addr=%p, written=%lu, total=%lu, sta=%d]",
                 store, (unsigned long)store->total_written, (unsigned long)store->file_total, store->cur_sta);

        // 确保临时文件被关闭
        if (store->temp_file)
        {
            fclose(store->temp_file);
            store->temp_file = NULL;
        }

        {
            int sock_err = esp_http_client_get_errno(evt->client);
            int tls_err = 0, tls_flags = 0;
            esp_http_client_get_and_clear_last_tls_error(evt->client, &tls_err, &tls_flags);

            ESP_LOGI(TAG, "errno=%d, tls_err=0x%x, tls_flags=0x%x", sock_err, tls_err, tls_flags);

            if (sock_err == 113 && tls_err == 0 && tls_flags == 0)
            {
                ESP_LOGE(TAG, "下载失败，网络连接超时");
                ESP_LOGI(TAG, "cur_sta=%d", store->cur_sta);
            }

            // 非成功结束时，确保通知UI退出进度页
            if (store->cur_sta != 2)
            {
                ui_fail_reason_t reason = UI_FAIL_NET_DISCONNECT;
                if (store->cur_sta == DL_STA_NO_SPACE)
                {
                    reason = UI_FAIL_NO_SPACE;
                }
                else if (tls_err != 0 || tls_flags != 0)
                {
                    reason = UI_FAIL_TLS_ERROR;
                }
                else if (sock_err == ETIMEDOUT)
                {
                    reason = UI_FAIL_TIMEOUT;
                }
                download_progress_fail(reason);
            }

            // 下载失败时删除临时文件
            if (store->cur_sta != 2 && store->temp_path)
            {
                ESP_LOGI(TAG, "清理临时文件: %s", store->temp_path);
                unlink(store->temp_path);
            }
        }
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }

    return ESP_OK;
}

/*
    http事件回调下载方式 - 直接流式写入SD卡
*/
static doit_file_result_t http_download_chunk(const char *file_url, const char *dir_name)
{
    doit_file_result_t ret = {.err_code = CL_OPRET_SUCCESS, .path = NULL, .type = NULL};

    /* 1. 取文件类型 */
    char *file_type = get_file_type_in_url(file_url);
    if (!file_type)
    {
        ret.err_code = CL_OPERT_FAIL;
        download_progress_fail(UI_FAIL_NO_PSRAM);
        return ret;
    }

    /* 2. 确定材料类型并获取下一个索引 */
    material_type_t type = (strcmp(dir_name, "animal") == 0) ? MATERIAL_TYPE_VPG : MATERIAL_TYPE_IMG;
    uint32_t next_index = doit_file_get_next_download_index(type);

    /* 3. 拼接SD卡完整路径：/sdcard/{dir_name}/{02}.{ext} */
    char *full_path = malloc(64);
    char *temp_path = malloc(64);
    if (!full_path || !temp_path)
    {
        free(file_type);
        if (full_path)
            free(full_path);
        if (temp_path)
            free(temp_path);
        ret.err_code = CL_OPERT_FAIL;
        download_progress_fail(UI_FAIL_NO_PSRAM);
        return ret;
    }
    snprintf(full_path, 64, "/sdcard/%s/%02lu.%s", dir_name, (unsigned long)next_index, file_type);
    // 临时文件放在同目录下，使用 .tmp 后缀
    snprintf(temp_path, 64, "/sdcard/%s/%02lu.tmp", dir_name, (unsigned long)next_index);

    /* 4. 初始化保存结构 */
    http_save_file_t save = {0};
    save.final_path = full_path;
    save.temp_path = temp_path;
    save.cur_sta = 0;
    save.total_written = 0;
    save.temp_file = NULL;

    /* 5. 配置HTTP客户端 */
    esp_http_client_config_t config = {
        .url = file_url,
        .event_handler = http_event_handler,
        .user_data = &save,
        .buffer_size = HTTP_DOWNLOAD_CHUNK_BUFFER,
        .keep_alive_enable = true,
        .timeout_ms = 30000,
        .disable_auto_redirect = false,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
    };

    /* 6. 执行下载 */
    ESP_LOGI(TAG, "HTTP download: temp=%s, final=%s", temp_path, full_path);
    ESP_LOGI(TAG, "save结构体地址: %p, 初始值: written=%lu, total=%lu, sta=%d",
             &save, (unsigned long)save.total_written, (unsigned long)save.file_total, save.cur_sta);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(TAG, "esp_http_client_init failed");
        ret.err_code = CL_OPERT_FAIL;
        download_progress_fail(UI_FAIL_HTTP_ERROR);
        free(file_type);
        free(full_path);
        free(temp_path);
        return ret;
    }
    esp_err_t err = esp_http_client_perform(client);

    int status_code = esp_http_client_get_status_code(client);
    int64_t content_length = esp_http_client_get_content_length(client);

    ESP_LOGI(TAG, "HTTP Status=%d, content_length=%d, written=%lu",
             status_code, (int)content_length, (unsigned long)save.total_written);
    ESP_LOGI(TAG, "下载完成后save状态: addr=%p, written=%lu, total=%lu, sta=%d",
             &save, (unsigned long)save.total_written, (unsigned long)save.file_total, save.cur_sta);

    /* 7. 验证下载完整性 */
    if (err == ESP_OK && status_code == 200 && save.cur_sta == 2)
    {
        ret.path = strdup(full_path);
        ret.type = strdup(file_type);
        ESP_LOGI(TAG, "Download success: %lu bytes -> %s",
                 (unsigned long)save.total_written, full_path);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP failed: err=%s, status=%d, sta=%d, written=%lu, total=%lu",
                 esp_err_to_name(err), status_code, save.cur_sta,
                 (unsigned long)save.total_written, (unsigned long)save.file_total);
        ret.err_code = CL_OPERT_FAIL;

        // 兜底：若事件回调未送达FAIL，主动通知一次，避免进度页悬挂
        if (save.cur_sta != 2)
        {
            ui_fail_reason_t reason = UI_FAIL_NET_DISCONNECT;
            if (save.cur_sta == DL_STA_NO_SPACE)
            {
                reason = UI_FAIL_NO_SPACE;
            }
            else if (status_code > 0 && status_code != 200)
            {
                reason = UI_FAIL_HTTP_ERROR;
            }
            else if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_HTTP_CONNECT)
            {
                reason = UI_FAIL_TIMEOUT;
            }
            download_progress_fail(reason);
        }

        unlink(temp_path); // 删除临时文件
    }

    /* 8. 清理 */
    if (save.temp_file)
    {
        fclose(save.temp_file);
        save.temp_file = NULL;
    }
    esp_http_client_cleanup(client);
    free(file_type);
    free(full_path);
    free(temp_path);

    return ret;
}

/* 检查文件大小是否超限 */
static bool is_http_file_content_length_overflow(const char *url)
{
    ESP_LOGI(TAG, "Checking file size for URL: %s", url);
    bool ret = false;

    s_content_len = 0;
    esp_http_client_config_t head_cfg = {
        .url = url,
        .method = HTTP_METHOD_HEAD,
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
    };

    esp_http_client_handle_t head = esp_http_client_init(&head_cfg);
    if (!head)
    {
        ESP_LOGW(TAG, "HEAD client init failed, continue download");
        return false;
    }

    esp_err_t head_err = esp_http_client_perform(head);
    if (head_err == ESP_OK)
    {
        int64_t content_len = esp_http_client_get_content_length(head);
        if (content_len <= 0)
        {
            ESP_LOGW(TAG, "HEAD has no valid Content-Length, continue download");
            ret = false;
        }
        else
        {
            s_content_len = content_len;
            ESP_LOGI(TAG, "File size: %d bytes", (int)s_content_len);
            if (s_content_len > HTTP_DOWNLOAD_MAX_SIZE)
            {
                ESP_LOGW(TAG, "File size exceeds limit of %d bytes, skip download", HTTP_DOWNLOAD_MAX_SIZE);
                ret = true;
            }
            else
            {
                ret = false;
            }
        }
    }
    else
    {
        ESP_LOGW(TAG, "HEAD request failed (%s), continue download", esp_err_to_name(head_err));
        ret = false;
    }

    esp_http_client_cleanup(head);
    return ret;
}

static doit_file_result_t to_download(const char *url, const char *dir_name)
{
    // 创建下载进度条
    if (!download_progress_create())
        return (doit_file_result_t){.err_code = CL_OPERT_FAIL, .path = NULL, .type = NULL};

    return http_download_chunk(url, dir_name);
}

static char *get_file_type_in_url(const char *url)
{
    if (!url)
        return strdup("bin");

    const char *filename = strrchr(url, '/');
    filename = filename ? (filename + 1) : url;

    const char *dot = strrchr(filename, '.');
    if (dot && *(dot + 1) != '\0')
    {
        const char *ext_start = dot + 1;
        const char *ext_end = ext_start;
        while (*ext_end && *ext_end != '?' && *ext_end != '#')
        {
            ext_end++;
        }

        size_t ext_len = (size_t)(ext_end - ext_start);
        if (ext_len > 0 && ext_len < 16)
        {
            for (size_t i = 0; i < ext_len; i++)
            {
                if (!isalnum((unsigned char)ext_start[i]))
                {
                    return strdup("bin");
                }
            }

            char *ext = (char *)malloc(ext_len + 1);
            if (!ext)
                return strdup("bin");
            for (size_t i = 0; i < ext_len; i++)
            {
                ext[i] = (char)tolower((unsigned char)ext_start[i]);
            }
            ext[ext_len] = '\0';
            ESP_LOGI(TAG, "get file type is %s in url %s", ext, url);
            return ext;
        }
    }
    return strdup("bin");
}

static uint8_t calc_download_percent(uint32_t written, uint32_t total)
{
    if (total == 0)
        return 0;

    uint64_t pct = ((uint64_t)written * 100ULL) / (uint64_t)total;
    if (pct > 100ULL)
        pct = 100ULL;
    return (uint8_t)pct;
}

static bool is_supported_download_dir(const char *dir_name)
{
    if (!dir_name)
        return false;
    return (strcmp(dir_name, "animal") == 0 || strcmp(dir_name, "photo") == 0);
}

/* 下载文件到SD卡 */
doit_file_result_t doit_file_download(const char *url, const char *dir_name)
{
    if (!url || url[0] == '\0' || !is_supported_download_dir(dir_name))
    {
        ESP_LOGE(TAG, "Invalid download params: url=%s, dir=%s",
                 url ? url : "NULL", dir_name ? dir_name : "NULL");
        return (doit_file_result_t){.err_code = CL_OPERT_FAIL, .path = NULL, .type = NULL};
    }
    if (strncmp(url, "http://", 7) == 0)
    {
        ESP_LOGE(TAG, "Insecure URL is not allowed: %s", url);
        return (doit_file_result_t){.err_code = CL_OPRET_NET_ERR, .path = NULL, .type = NULL};
    }

    doit_vpg_player_stop();
    ESP_LOGI(TAG, "Downloading file from: %s", url);

    doit_file_result_t ret = {.err_code = CL_OPRET_SUCCESS, .path = NULL, .type = NULL};
    bool expected_busy = false;
    if (!__atomic_compare_exchange_n(&s_download_busy, &expected_busy, true, false,
                                     __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        ESP_LOGW(TAG, "Download busy, reject new request");
        ret.err_code = CL_OPRET_BUSY;
        return ret;
    }

    if (is_http_file_content_length_overflow(url))
    {
        download_fail_show_reason_toast(UI_FAIL_HTTP_ERROR);
        ret.err_code = CL_OPRET_FILE_OVERFLOW;
        __atomic_store_n(&s_download_busy, false, __ATOMIC_RELEASE);
        return ret;
    }

    if (s_content_len > 0)
    {
        uint64_t fs_total_bytes = 0;
        uint64_t fs_free_bytes = 0;
        if (esp_vfs_fat_info("/sdcard", &fs_total_bytes, &fs_free_bytes) == ESP_OK)
        {
            if ((uint64_t)s_content_len > fs_free_bytes)
            {
                ESP_LOGE(TAG, "SD卡容量不足(预检查): need=%d bytes, free=%u bytes, total=%u bytes",
                         (int)s_content_len, (unsigned int)fs_free_bytes, (unsigned int)fs_total_bytes);
                download_fail_show_reason_toast(UI_FAIL_NO_SPACE);
                ret.err_code = CL_OPERT_FAIL;
                __atomic_store_n(&s_download_busy, false, __ATOMIC_RELEASE);
                return ret;
            }
        }
    }

    doit_file_result_t download_ret = to_download(url, dir_name);
    ret.err_code = download_ret.err_code;
    ret.path = download_ret.path;
    ret.type = download_ret.type;

    __atomic_store_n(&s_download_busy, false, __ATOMIC_RELEASE);
    return ret;
}
