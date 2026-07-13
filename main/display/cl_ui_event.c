#include "cl_ui_event.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cl_ui.h"

#ifndef SIMULATOR
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#else
#include <pthread.h>
#include <errno.h>
#include <time.h>
#endif


static const char *TAG = "lv_event_sys";

#ifndef SIMULATOR
typedef struct event_node {
    lv_event_code_t code;
    void *param;              // 拷贝后的参数
    uint32_t param_len;

    lv_event_system_route_cb_t cb;
    void *user_ctx;

    bool is_sync;
    SemaphoreHandle_t done_sem;   // 同步发送用
    esp_err_t result;             // 预留，路由回调可以写结果

    struct event_node *next;
} event_node_t;

/* 全局状态 */
static TaskHandle_t        s_event_task      = NULL;
static SemaphoreHandle_t   s_list_lock       = NULL;  // 保护链表
static SemaphoreHandle_t   s_list_sem        = NULL;  // 通知有新事件
static event_node_t       *s_list_head       = NULL;
static event_node_t       *s_list_tail       = NULL;
static bool                s_inited          = false;
static bool                s_task_exit       = false;

/* ------- 内部工具函数 ------- */

static void list_push(event_node_t *node)
{
    node->next = NULL;
    if (!s_list_head) {
        s_list_head = s_list_tail = node;
    } else {
        s_list_tail->next = node;
        s_list_tail = node;
    }
}

static event_node_t *list_pop(void)
{
    event_node_t *node = s_list_head;
    if (!node) return NULL;

    s_list_head = node->next;
    if (!s_list_head) {
        s_list_tail = NULL;
    }
    node->next = NULL;
    return node;
}

/**
 * 事件处理任务：
 *  - 永远在同一个上下文中执行路由回调（你可以在回调里加 lvgl_lock/unlock）
 */
static void lv_event_worker_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "event task start");

    while (!s_task_exit) {
        // 等待有新事件
        if (xSemaphoreTake(s_list_sem, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        while (1) {
            xSemaphoreTake(s_list_lock, portMAX_DELAY);
            event_node_t *node = list_pop();
            xSemaphoreGive(s_list_lock);

            if (!node) break;

            // 调用路由回调
            if (node->cb) {
                node->cb(node->code, node->param, node->param_len, node->user_ctx);
                node->result = ESP_OK;
            } else {
                ESP_LOGW(TAG, "no route callback for event %d", node->code);
                node->result = ESP_ERR_INVALID_STATE;
            }

            // 同步事件：唤醒等待者
            if (node->is_sync && node->done_sem) {
                xSemaphoreGive(node->done_sem);
            }

            // 释放 param
            if (node->param) {
                free(node->param);
                node->param = NULL;
            }

            // 同步事件：等待方释放 done_sem 和 node
            if (!node->is_sync) {
                if (node->done_sem) {
                    vSemaphoreDelete(node->done_sem);
                }
                free(node);
            }
        }
    }

    ESP_LOGI(TAG, "event task exit");
    vTaskDelete(NULL);
}

/* ------- 对外接口实现 ------- */

esp_err_t lv_event_system_init(uint32_t task_stack, UBaseType_t priority)
{
    if (s_inited) {
        return ESP_OK;
    }

    s_list_lock = xSemaphoreCreateMutex();
    if (!s_list_lock) {
        ESP_LOGE(TAG, "create list mutex failed");
        return ESP_ERR_NO_MEM;
    }

    s_list_sem = xSemaphoreCreateBinary();
    if (!s_list_sem) {
        ESP_LOGE(TAG, "create list sem failed");
        vSemaphoreDelete(s_list_lock);
        s_list_lock = NULL;
        return ESP_ERR_NO_MEM;
    }

    s_task_exit = false;

    BaseType_t ret = xTaskCreate(
        lv_event_worker_task,
        "lv_event_worker",
        task_stack,
        NULL,
        priority,
        &s_event_task
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "create event task failed");
        vSemaphoreDelete(s_list_lock);
        vSemaphoreDelete(s_list_sem);
        s_list_lock = NULL;
        s_list_sem  = NULL;
        return ESP_FAIL;
    }

    s_inited = true;
    return ESP_OK;
}

esp_err_t lv_event_system_deinit(void)
{
    if (!s_inited) {
        return ESP_OK;
    }

    s_task_exit = true;
    if (s_list_sem) {
        xSemaphoreGive(s_list_sem);  // 唤醒任务退出
    }

    // 注意：这里简单处理，没有等任务真正结束，如果需要可以加事件同步
    s_event_task = NULL;

    // 清空链表
    xSemaphoreTake(s_list_lock, portMAX_DELAY);
    event_node_t *node = NULL;
    while ((node = list_pop()) != NULL) {
        if (node->param) free(node->param);
        if (node->done_sem) vSemaphoreDelete(node->done_sem);
        free(node);
    }
    xSemaphoreGive(s_list_lock);

    vSemaphoreDelete(s_list_lock);
    vSemaphoreDelete(s_list_sem);

    s_list_lock = NULL;
    s_list_sem  = NULL;
    s_inited    = false;

    return ESP_OK;
}

/**
 * 内部共用：构造一个 node 并加入链表
 */
static esp_err_t enqueue_event(lv_event_code_t code,
                               const void *param,
                               uint32_t param_len,
                               lv_event_system_route_cb_t cb,
                               void *user_ctx,
                               bool is_sync,
                               SemaphoreHandle_t sync_sem)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "lv_event_system not inited");
        return ESP_ERR_INVALID_STATE;
    }

    event_node_t *node = (event_node_t *)calloc(1, sizeof(event_node_t));
    if (!node) {
        return ESP_ERR_NO_MEM;
    }

    node->code = code;
    node->cb   = cb;
    node->user_ctx = user_ctx;
    node->is_sync  = is_sync;
    node->done_sem = sync_sem;
    node->result   = ESP_FAIL;

    if (param && param_len > 0) {
        node->param = malloc(param_len);
        if (!node->param) {
            free(node);
            return ESP_ERR_NO_MEM;
        }
        memcpy(node->param, param, param_len);
        node->param_len = param_len;
    } else {
        node->param = NULL;
        node->param_len = 0;
    }

    xSemaphoreTake(s_list_lock, portMAX_DELAY);
    list_push(node);
    xSemaphoreGive(s_list_lock);

    xSemaphoreGive(s_list_sem);  // 通知任务

    return ESP_OK;
}

esp_err_t lv_event_system_send_async(lv_event_code_t code,
                                     const void *param,
                                     uint32_t param_len,
                                     lv_event_system_route_cb_t cb,
                                     void *user_ctx)
{
    if (!cb) {
        return ESP_ERR_INVALID_ARG;
    }

    return enqueue_event(code, param, param_len, cb, user_ctx, false, NULL);
}

esp_err_t lv_event_system_send_sync(lv_event_code_t code,
                                    const void *param,
                                    uint32_t param_len,
                                    lv_event_system_route_cb_t cb,
                                    void *user_ctx,
                                    TickType_t timeout)
{
    if (!cb) {
        return ESP_ERR_INVALID_ARG;
    }

    SemaphoreHandle_t sync_sem = xSemaphoreCreateBinary();
    if (!sync_sem) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = enqueue_event(code, param, param_len, cb, user_ctx, true, sync_sem);
    if (err != ESP_OK) {
        vSemaphoreDelete(sync_sem);
        return err;
    }

    // 等待事件处理完成
    if (xSemaphoreTake(sync_sem, timeout) != pdTRUE) {
        ESP_LOGW(TAG, "wait event sync timeout");
        vSemaphoreDelete(sync_sem);
        return ESP_ERR_TIMEOUT;
    }

    // 注意：node->result 在本实现里没有返回途径，如果要用，可以扩展：
    //  - 比如把 result 塞到 user_ctx 指向的结构里
    vSemaphoreDelete(sync_sem);
    return ESP_OK;
}
#else /* SIMULATOR */

typedef struct event_waiter {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    uint32_t refcnt;
} event_waiter_t;

typedef struct event_node {
    lv_event_code_t code;
    void *param;
    uint32_t param_len;

    lv_event_system_route_cb_t cb;
    void *user_ctx;

    event_waiter_t *waiter;
    struct event_node *next;
} event_node_t;

static pthread_t           s_event_thread;
static pthread_mutex_t     s_list_mutex;
static pthread_cond_t      s_list_cond;
static event_node_t       *s_list_head = NULL;
static event_node_t       *s_list_tail = NULL;
static bool                s_inited    = false;
static bool                s_task_exit = false;

static event_waiter_t *waiter_create(void)
{
    event_waiter_t *waiter = (event_waiter_t *)calloc(1, sizeof(event_waiter_t));
    if (!waiter) {
        return NULL;
    }

    if (pthread_mutex_init(&waiter->mutex, NULL) != 0) {
        free(waiter);
        return NULL;
    }

    if (pthread_cond_init(&waiter->cond, NULL) != 0) {
        pthread_mutex_destroy(&waiter->mutex);
        free(waiter);
        return NULL;
    }

    waiter->done = false;
    waiter->refcnt = 1;
    return waiter;
}

static void waiter_retain(event_waiter_t *waiter)
{
    if (!waiter) {
        return;
    }

    pthread_mutex_lock(&waiter->mutex);
    waiter->refcnt++;
    pthread_mutex_unlock(&waiter->mutex);
}

static void waiter_release(event_waiter_t *waiter)
{
    if (!waiter) {
        return;
    }

    bool destroy = false;
    pthread_mutex_lock(&waiter->mutex);
    if (waiter->refcnt > 0) {
        waiter->refcnt--;
        if (waiter->refcnt == 0) {
            destroy = true;
        }
    }
    pthread_mutex_unlock(&waiter->mutex);

    if (destroy) {
        pthread_cond_destroy(&waiter->cond);
        pthread_mutex_destroy(&waiter->mutex);
        free(waiter);
    }
}

static void list_push(event_node_t *node)
{
    node->next = NULL;
    if (!s_list_head) {
        s_list_head = s_list_tail = node;
    } else {
        s_list_tail->next = node;
        s_list_tail = node;
    }
}

static event_node_t *list_pop(void)
{
    event_node_t *node = s_list_head;
    if (!node) {
        return NULL;
    }

    s_list_head = node->next;
    if (!s_list_head) {
        s_list_tail = NULL;
    }
    node->next = NULL;
    return node;
}

static void *lv_event_worker_thread(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "event worker start");

    while (true) {
        pthread_mutex_lock(&s_list_mutex);
        while (!s_task_exit && !s_list_head) {
            pthread_cond_wait(&s_list_cond, &s_list_mutex);
        }

        if (s_task_exit && !s_list_head) {
            pthread_mutex_unlock(&s_list_mutex);
            break;
        }

        event_node_t *node = list_pop();
        pthread_mutex_unlock(&s_list_mutex);

        if (!node) {
            continue;
        }

        if (node->cb) {
            node->cb(node->code, node->param, node->param_len, node->user_ctx);
        } else {
            ESP_LOGW(TAG, "no route callback for event %d", node->code);
        }

        if (node->waiter) {
            pthread_mutex_lock(&node->waiter->mutex);
            node->waiter->done = true;
            pthread_cond_signal(&node->waiter->cond);
            pthread_mutex_unlock(&node->waiter->mutex);
            waiter_release(node->waiter);
        }

        if (node->param) {
            free(node->param);
        }
        free(node);
    }

    ESP_LOGI(TAG, "event worker exit");
    return NULL;
}

static esp_err_t enqueue_event(lv_event_code_t code,
                               const void *param,
                               uint32_t param_len,
                               lv_event_system_route_cb_t cb,
                               void *user_ctx,
                               event_waiter_t *waiter)
{
    if (!s_inited) {
        ESP_LOGE(TAG, "lv_event_system not inited");
        return ESP_ERR_INVALID_STATE;
    }

    event_node_t *node = (event_node_t *)calloc(1, sizeof(event_node_t));
    if (!node) {
        return ESP_ERR_NO_MEM;
    }

    node->code = code;
    node->cb = cb;
    node->user_ctx = user_ctx;
    node->waiter = waiter;
    if (node->waiter) {
        waiter_retain(node->waiter);
    }

    if (param && param_len > 0) {
        node->param = malloc(param_len);
        if (!node->param) {
            free(node);
            return ESP_ERR_NO_MEM;
        }
        memcpy(node->param, param, param_len);
        node->param_len = param_len;
    } else {
        node->param = NULL;
        node->param_len = 0;
    }

    pthread_mutex_lock(&s_list_mutex);
    list_push(node);
    pthread_mutex_unlock(&s_list_mutex);
    pthread_cond_signal(&s_list_cond);

    return ESP_OK;
}

esp_err_t lv_event_system_init(uint32_t task_stack, UBaseType_t priority)
{
    (void)task_stack;
    (void)priority;

    if (s_inited) {
        return ESP_OK;
    }

    if (pthread_mutex_init(&s_list_mutex, NULL) != 0) {
        ESP_LOGE(TAG, "init mutex failed");
        return ESP_FAIL;
    }

    if (pthread_cond_init(&s_list_cond, NULL) != 0) {
        ESP_LOGE(TAG, "init cond failed");
        pthread_mutex_destroy(&s_list_mutex);
        return ESP_FAIL;
    }

    s_task_exit = false;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (task_stack > 0) {
        pthread_attr_setstacksize(&attr, task_stack);
    }

    int ret = pthread_create(&s_event_thread, &attr, lv_event_worker_thread, NULL);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        ESP_LOGE(TAG, "create thread failed, errno=%d", ret);
        pthread_cond_destroy(&s_list_cond);
        pthread_mutex_destroy(&s_list_mutex);
        return ESP_FAIL;
    }

    s_inited = true;
    return ESP_OK;
}

esp_err_t lv_event_system_deinit(void)
{
    if (!s_inited) {
        return ESP_OK;
    }

    pthread_mutex_lock(&s_list_mutex);
    s_task_exit = true;
    pthread_cond_broadcast(&s_list_cond);
    pthread_mutex_unlock(&s_list_mutex);

    pthread_join(s_event_thread, NULL);

    pthread_mutex_lock(&s_list_mutex);
    event_node_t *node = NULL;
    while ((node = list_pop()) != NULL) {
        if (node->param) {
            free(node->param);
        }
        if (node->waiter) {
            pthread_mutex_lock(&node->waiter->mutex);
            node->waiter->done = true;
            pthread_cond_signal(&node->waiter->cond);
            pthread_mutex_unlock(&node->waiter->mutex);
            waiter_release(node->waiter);
        }
        free(node);
    }
    pthread_mutex_unlock(&s_list_mutex);

    pthread_cond_destroy(&s_list_cond);
    pthread_mutex_destroy(&s_list_mutex);

    s_inited = false;
    s_list_head = s_list_tail = NULL;
    return ESP_OK;
}

esp_err_t lv_event_system_send_async(lv_event_code_t code,
                                     const void *param,
                                     uint32_t param_len,
                                     lv_event_system_route_cb_t cb,
                                     void *user_ctx)
{
    if (!cb) {
        return ESP_ERR_INVALID_ARG;
    }

    return enqueue_event(code, param, param_len, cb, user_ctx, NULL);
}

static struct timespec calc_deadline(TickType_t timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ns = (uint64_t)timeout_ms * 1000000ULL;
    ts.tv_sec += ns / 1000000000ULL;
    ts.tv_nsec += ns % 1000000000ULL;
    if (ts.tv_nsec >= 1000000000ULL) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000ULL;
    }
    return ts;
}

static esp_err_t wait_event_done(event_waiter_t *waiter, TickType_t timeout_ms)
{
    if (!waiter) {
        return ESP_FAIL;
    }

    pthread_mutex_lock(&waiter->mutex);
    esp_err_t err = ESP_OK;

    bool use_deadline = (timeout_ms != portMAX_DELAY);
    struct timespec deadline;
    if (use_deadline) {
        deadline = calc_deadline(timeout_ms);
    }

    while (!waiter->done) {
        int rc;
        if (!use_deadline) {
            rc = pthread_cond_wait(&waiter->cond, &waiter->mutex);
            if (rc != 0) {
                err = ESP_FAIL;
                break;
            }
        } else {
            rc = pthread_cond_timedwait(&waiter->cond, &waiter->mutex, &deadline);
            if (rc == ETIMEDOUT) {
                err = ESP_ERR_TIMEOUT;
                break;
            } else if (rc != 0) {
                err = ESP_FAIL;
                break;
            }
        }
    }

    if (waiter->done) {
        err = ESP_OK;
    }

    pthread_mutex_unlock(&waiter->mutex);
    return err;
}

esp_err_t lv_event_system_send_sync(lv_event_code_t code,
                                    const void *param,
                                    uint32_t param_len,
                                    lv_event_system_route_cb_t cb,
                                    void *user_ctx,
                                    TickType_t timeout)
{
    if (!cb) {
        return ESP_ERR_INVALID_ARG;
    }

    event_waiter_t *waiter = waiter_create();
    if (!waiter) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = enqueue_event(code, param, param_len, cb, user_ctx, waiter);
    if (err != ESP_OK) {
        waiter_release(waiter);
        return err;
    }

    err = wait_event_done(waiter, timeout);
    waiter_release(waiter);
    return err;
}

#endif /* SIMULATOR */
