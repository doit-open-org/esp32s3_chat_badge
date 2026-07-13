#pragma once

#include "lvgl.h"
#ifndef SIMULATOR
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#else
#include <stdint.h>

#define esp_err_t int
#define UBaseType_t int
typedef uint32_t TickType_t;

#define ESP_OK                  (0)
#define ESP_FAIL                (-1)
#define ESP_ERR_NO_MEM          (0x101)
#define ESP_ERR_INVALID_STATE   (0x102)
#define ESP_ERR_INVALID_ARG     (0x103)
#define ESP_ERR_TIMEOUT         (0x107)

#ifndef portMAX_DELAY
#define portMAX_DELAY           (UINT32_MAX)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 路由回调：
 * - 在「事件线程 / GUI 线程」里被调用
 * - 你在这个回调里决定把事件发给哪个 lv_obj_t
 *   一般就是：lv_event_send(target, code, param)
 */
typedef void (*lv_event_system_route_cb_t)(lv_event_code_t code,
                                           void *param,
                                           uint32_t param_len,
                                           void *user_ctx);

/**
 * 初始化事件系统：
 *  - 创建事件处理任务
 *  - task_stack: 任务栈大小
 *  - priority:   任务优先级
 */
esp_err_t lv_event_system_init(uint32_t task_stack, UBaseType_t priority);

/** 反初始化（可选） */
esp_err_t lv_event_system_deinit(void);

/**
 * 异步发送事件：
 *  - 只负责排队，不等待处理完成
 *  - param 会被拷贝
 */
esp_err_t lv_event_system_send_async(lv_event_code_t code,
                                     const void *param,
                                     uint32_t param_len,
                                     lv_event_system_route_cb_t cb,
                                     void *user_ctx);

/**
 * 同步发送事件（仍由事件任务执行）：
 *  - 发送端会阻塞等待事件处理完成（等待 timeout）
 *  - 适合在别的任务里希望「保证 GUI 已处理完」
 */
esp_err_t lv_event_system_send_sync(lv_event_code_t code,
                                    const void *param,
                                    uint32_t param_len,
                                    lv_event_system_route_cb_t cb,
                                    void *user_ctx,
                                    TickType_t timeout);

/**
 * 直接同步发送（不入队，立即在当前上下文调用路由回调）
 *  - 一般在 GUI/LVGL 任务中使用
 *  - param 由调用方自己管理（本函数不拷贝、不释放）
 */
static inline esp_err_t lv_event_system_send_direct(lv_event_code_t code,
                                                    void *param,
                                                    uint32_t param_len,
                                                    lv_event_system_route_cb_t cb,
                                                    void *user_ctx)
{
    if (!cb) {
        return ESP_ERR_INVALID_ARG;
    }
    cb(code, param, param_len, user_ctx);
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
