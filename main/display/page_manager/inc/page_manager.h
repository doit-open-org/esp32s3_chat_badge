#pragma once

#include "adlist.h"
#include "page_base.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Page switching animation type  */
    typedef enum
    {
        /* Default (global) animation type  */
        LOAD_ANIM_GLOBAL = 0,

        /* New page overwrites old page  */
        LOAD_ANIM_OVER_LEFT,
        LOAD_ANIM_OVER_RIGHT,
        LOAD_ANIM_OVER_TOP,
        LOAD_ANIM_OVER_BOTTOM,

        /* New page pushes old page  */
        LOAD_ANIM_MOVE_LEFT,
        LOAD_ANIM_MOVE_RIGHT,
        LOAD_ANIM_MOVE_TOP,
        LOAD_ANIM_MOVE_BOTTOM,

        /* The new interface fades in, the old page fades out */
        LOAD_ANIM_FADE_ON,

        /* No animation */
        LOAD_ANIM_NONE,

        _LOAD_ANIM_LAST = LOAD_ANIM_NONE
    } page_load_anim_t;

    /* Page dragging direction */
    typedef enum
    {
        ROOT_DRAG_DIR_NONE,
        ROOT_DRAG_DIR_HOR,
        ROOT_DRAG_DIR_VER,
    } page_root_drag_dir_t;

    /* Animated setter  */
    typedef lv_anim_exec_xcb_t lv_anim_setter_t;

    /* Animated getter  */
    typedef int32_t (*lv_anim_getter_t)(void *);

    /* Animation switching record  */
    typedef struct
    {
        /* As the entered party */
        struct
        {
            int32_t start;
            int32_t end;
        } enter;

        /* As the exited party */
        struct
        {
            int32_t start;
            int32_t end;
        } exit;
    } page_anim_value_t;

    /* 页面加载动画属性 */
    typedef struct
    {
        lv_anim_setter_t setter;
        lv_anim_getter_t getter;
        page_root_drag_dir_t drag_dir;
        page_anim_value_t push;
        page_anim_value_t pop;
    } page_load_anim_attr_t;

    typedef struct page_manager_t
    {
        list *page_pool;           // 页面池，用于注册页面
        list *page_stack;          // 页面堆栈，用于收集页面进入方式并依次退出
        page_base_t *page_prev;    // 上一个页面节点
        page_base_t *page_current; // 当前页面节点
        struct
        {
            bool is_switch_req;       // 是否切换请求
            bool is_busy;             // 忙碌标志位
            bool is_pushing;          // 是否处于压栈状态
            page_anim_attr_t current; // 当前动画属性
            page_anim_attr_t global;  // 全局动画属性
        } anim_state;
    } page_manager_t;

    /**
     * @brief 创建页面管理器对象
     *
     * @return page_manager_t* 页面管理器对象
     */
    page_manager_t *page_manager_create(void);

    /**
     * @brief 删除页面管理器对象
     *
     * @param self 页面管理器对象
     */
    void page_manager_delete(page_manager_t *self);

    /**
     * @brief 安装页面到页面管理器中
     *
     * @param self 页面管理器对象
     * @param page_param 页面调度函数
     */
    void pm_install(page_manager_t *self, const char *name, page_vtable_t* page_param);

    /**
     * @brief 页面管理器中卸载页面
     *
     * @param self 页面管理器对象
     * @param name 页面名称
     */
    void pm_uninstall(page_manager_t *self, const char *name);

    /**
     * @brief 页面管理器中加载页面展示
     *
     * @param self 页面管理器对象
     * @param name 页面名称
     * @param stash 缓存区,没有数据就填NULL
     */
    void pm_push(page_manager_t *self, const char *name, const page_stash_t *stash);

    /**
     * @brief 回退到上一个页面
     *
     * @param self 页面管理器对象
     */
    void pm_pop(page_manager_t *self);

    /**
     * @brief 返回主界面
     *
     * @param self 页面管理器对象
     * @return true 返回成功
     * @return false 页面正在切换
     */
    bool pm_back_home(page_manager_t *self);

    /**
     * @brief 设置页面管理器全局默认动画参数
     *
     * @param self 页面管理器对象
     * @param anim 动画类型
     * @param time 动画持续时间
     * @param path 动画路径
     */
    void pm_set_global_load_anim_type(page_manager_t *self, page_load_anim_t anim, uint16_t time, lv_anim_path_cb_t path);


    /**
     * @brief 获取上一个页面名称
     *
     * @param self 页面管理器对象
     * @return const char* 上一个页面名称
     */
    const char* pm_get_last_page_name(page_manager_t *self);

    /*
     * @brief 获取指定页面名称
     *
     * @param self 页面管理器对象
     * @param index 页面索引
     * @return const char* 指定页面名称
    */
    const char* pm_get_last_page_name_by_index(page_manager_t *self, size_t index);

    /**
     * @brief 回退到指定页面
     *
     * @param self 页面管理器对象
     * @param name 回退到该页面
     */
    void pm_pop_until(page_manager_t *self, const char *name);

    /**
     * @brief 获取指定页面
     *
     * @param self 页面管理器对象
     * @param name 页面名称
     * @return page_base_t* 指定页面
     */
    page_base_t *pm_find_page(page_manager_t *self, const char *name);

typedef struct
{
    const char *name;
    page_vtable_t* (*create)(const char *name);
    // int (*btn_fun)(user_button_t btn, xiaozhi_btn_event_t event, u8 repeat);
    void *page_arg;
} page_handle_t;

/* 统一定义“放到段里且不被丢弃”的属性 */
#if defined(__APPLE__)
  // macOS 走 Mach-O（见文末“跨平台”），这里先给 ELF 方案，macOS 见后面
  #define PAGE_SEC_USED __attribute__((section("__DATA,cl_ui_page"), used))
#elif defined(__ELF__) || defined(__linux__)
  #define PAGE_SEC_USED __attribute__((section("cl_ui_page"), used))
#else
  #define PAGE_SEC_USED __attribute__((section("cl_ui_page"), used))
#endif


/* 让你的 cl_ui_page_begin / cl_ui_page_end 直接映射到 ld 提供的符号 */
#if defined(__ELF__) || defined(__linux__)
  extern page_handle_t __start_cl_ui_page[];
  extern page_handle_t __stop_cl_ui_page[];
  #define cl_ui_page_begin __start_cl_ui_page
  #define cl_ui_page_end   __stop_cl_ui_page
#endif

#define REGIST_PAGE(n) \
    page_handle_t page_##n##_hd __attribute__((section("cl_ui_page"), used)) = { \
        .name = #n, \
        .create = n##_create, \
        .page_arg = (void*)&pg \
    };

#define GET_PAGE(n) extern page_handle_t page_##n##_hd
#define GET_PAGE_HD(n) page_##n##_hd
#define GET_PAGE_NAME(na) page_##na##_hd.name

#ifdef __cplusplus
} /* extern "C" */
#endif