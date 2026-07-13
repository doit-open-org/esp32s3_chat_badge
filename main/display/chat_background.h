/**
 * @file chat_background.h
 * @brief 聊天背景管理模块 - 负责背景的设置、获取和持久化
 */
#ifndef __CHAT_BACKGROUND_H__
#define __CHAT_BACKGROUND_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    // 背景类型枚举
    typedef enum
    {
        CHAT_BG_TYPE_NONE = 0, // 无背景（默认黑色）
        CHAT_BG_TYPE_IMAGE,    // 图片背景 (.jpg, .jpeg, .png, .bmp)
        CHAT_BG_TYPE_VPG       // VPG动画背景 (.vpg, .dat)
    } chat_bg_type_t;

 
    void chat_background_init(void);

   
    bool chat_background_set(const char *path);

  
    const char *chat_background_get_path(void);

    chat_bg_type_t chat_background_get_type(void);

  
    void chat_background_clear(void);

 
    bool chat_background_validate(void);


    bool chat_background_is_current(const char *path);

 
    bool chat_background_load_to_cache(const char *path, void **out_img_dsc);


    void chat_background_clear_cache(void);


    bool chat_background_is_cached(const char *path);

#ifdef __cplusplus
}
#endif

#endif // __CHAT_BACKGROUND_H__
