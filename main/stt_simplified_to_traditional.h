#ifndef _STT_SIMPLIFIED_TO_TRADITIONAL_H_
#define _STT_SIMPLIFIED_TO_TRADITIONAL_H_
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>

#define TAG "SttSimplifiedToTraditional"

#define S2T_URL "http://115.190.79.242:9801/api/user/ota/translate/cc"
static char response_data[256];

class SttSimplifiedToTraditional {
private:
static esp_err_t http_client_event_handler(esp_http_client_event_t *evt) {
  static int received_len = 0; // 记录已接收数据长度

  switch (evt->event_id) {
  case HTTP_EVENT_ON_CONNECTED:
    received_len = 0; // 清除之前的缓存
    memset(response_data, 0, sizeof(response_data));
    break;

  case HTTP_EVENT_ON_DATA:
    if (evt->user_data) {
      // 确保不会超过 response_data 缓冲区大小
      int copy_len = evt->data_len;
      if (received_len + copy_len >= sizeof(response_data)) {
        copy_len = sizeof(response_data) - received_len - 1;
      }
      memcpy(evt->user_data + received_len, evt->data, copy_len);
      received_len += copy_len;
    }
    break;

  case HTTP_EVENT_ON_FINISH:
  case HTTP_EVENT_DISCONNECTED:
  case HTTP_EVENT_ERROR:
    received_len = 0; // 复位计数
    break;

  default:
    break;
  }
  return ESP_OK;
}

  static bool cc_s2t_translate(std::string &input, std::string &output) {
    // 配置 HTTP 客户端
    esp_http_client_config_t config = {
        .url = S2T_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000, // 设置超时时间
        .event_handler = http_client_event_handler,
        .user_data = response_data, // 指定缓冲区存储响应数据
        // .cert_pem = (char *)server_cert_pem_start, // 服务器证书
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
      ESP_LOGE(TAG, "Failed to initialize HTTP client");
      return false;
    }

    // 构造 JSON 请求体
    std::string post_data = "{\"text\":\"" + input + "\"}";
    printf("json:%s\n", post_data.c_str());
    esp_http_client_set_post_field(client, post_data.c_str(),
                                   post_data.length());
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // 发送 HTTP 请求
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      return false;
    }

    // 获取 HTTP 响应状态码
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP Status = %d, Content Length = %" PRId64, status_code,
             esp_http_client_get_content_length(client));

    // 检查 HTTP 响应状态
    if (status_code != 200) {
      ESP_LOGE(TAG, "Unexpected HTTP response code: %d", status_code);
      esp_http_client_cleanup(client);
      return false;
    }

    ESP_LOGI(TAG, "Response Data: %s", response_data);

    bool success = false;

    cJSON *root = cJSON_Parse(response_data);
    if (root) {
      cJSON *data = cJSON_GetObjectItem(root, "traditional");
      if (data) {
        // 获取 bind_result
        if (cJSON_IsString(data)) {
          const char *traditional_chinese = cJSON_GetStringValue(data);
          output = std::string(traditional_chinese);
          success = true; // 成功解析到结果
        }
      } else {
        ESP_LOGE(TAG, "Invalid JSON format: missing 'data' field");
      }
      // 释放 JSON 对象
      cJSON_Delete(root);
    } else {
      ESP_LOGE(TAG, "Failed to parse JSON response");
    }

    // 清理客户端
    esp_http_client_cleanup(client);

    // 清空 response_data 以防数据污染
    memset(response_data, 0, sizeof(response_data));

    return success;
  }

public:
  SttSimplifiedToTraditional() {}

  static SttSimplifiedToTraditional *GetInstance() {
    static SttSimplifiedToTraditional instance;
    return &instance;
  }

  bool Translate(std::string &input, std::string &output) {
    return cc_s2t_translate(input, output);
  }
};

#endif