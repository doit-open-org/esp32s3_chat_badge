#include "vw_ai.h"
#include "cl_ui.h"
#include "lvgl.h"

static lv_obj_t *s_animimg = NULL;
static int emotion = 0;

LV_IMG_DECLARE(eye_static_00)
LV_IMG_DECLARE(eye_static_03)
LV_IMG_DECLARE(eye_static_02)
LV_IMG_DECLARE(eye_static_04)
LV_IMG_DECLARE(eye_static_05)

LV_IMG_DECLARE(eye_sad_loop_00)
LV_IMG_DECLARE(eye_sad_loop_01)
LV_IMG_DECLARE(eye_sad_loop_02)
LV_IMG_DECLARE(eye_sad_loop_03)
LV_IMG_DECLARE(eye_sad_loop_04)
LV_IMG_DECLARE(eye_sad_loop_05)
LV_IMG_DECLARE(eye_sad_loop_06)
LV_IMG_DECLARE(eye_sad_loop_07)
LV_IMG_DECLARE(eye_sad_loop_08)
LV_IMG_DECLARE(eye_sad_loop_09)
LV_IMG_DECLARE(eye_sad_loop_10)
LV_IMG_DECLARE(eye_sad_loop_11)
LV_IMG_DECLARE(eye_sad_loop_12)
LV_IMG_DECLARE(eye_sad_loop_13)
LV_IMG_DECLARE(eye_sad_loop_14)
LV_IMG_DECLARE(eye_sad_loop_15)
LV_IMG_DECLARE(eye_sad_loop_16)
LV_IMG_DECLARE(eye_sad_loop_17)

LV_IMG_DECLARE(eye_sad_exit_00)
LV_IMG_DECLARE(eye_sad_exit_01)
LV_IMG_DECLARE(eye_sad_exit_02)
LV_IMG_DECLARE(eye_sad_exit_03)
LV_IMG_DECLARE(eye_sad_exit_04)
LV_IMG_DECLARE(eye_sad_exit_05)
LV_IMG_DECLARE(eye_sad_exit_06)

LV_IMG_DECLARE(eye_happy_01)
LV_IMG_DECLARE(eye_happy_03)
LV_IMG_DECLARE(eye_happy_04)
LV_IMG_DECLARE(eye_happy_05)
LV_IMG_DECLARE(eye_happy_06)
LV_IMG_DECLARE(eye_happy_07)
LV_IMG_DECLARE(eye_happy_09)
LV_IMG_DECLARE(eye_happy_10)
LV_IMG_DECLARE(eye_happy_11)
LV_IMG_DECLARE(eye_happy_12)
LV_IMG_DECLARE(eye_happy_13)
LV_IMG_DECLARE(eye_happy_14)
LV_IMG_DECLARE(eye_happy_15)
LV_IMG_DECLARE(eye_happy_17)
LV_IMG_DECLARE(eye_happy_27)
LV_IMG_DECLARE(eye_happy_28)
LV_IMG_DECLARE(eye_happy_29)
LV_IMG_DECLARE(eye_happy_30)
LV_IMG_DECLARE(eye_happy_31)

LV_IMG_DECLARE(eye_scare_00)
LV_IMG_DECLARE(eye_scare_01)
LV_IMG_DECLARE(eye_scare_02)
LV_IMG_DECLARE(eye_scare_03)
LV_IMG_DECLARE(eye_scare_04)

LV_IMG_DECLARE(eye_scare_05)

LV_IMG_DECLARE(eye_scare_11)
LV_IMG_DECLARE(eye_scare_12)
LV_IMG_DECLARE(eye_scare_13)
LV_IMG_DECLARE(eye_scare_14)
LV_IMG_DECLARE(eye_scare_15)
LV_IMG_DECLARE(eye_scare_16)

LV_IMG_DECLARE(eye_scare_exit_00)
LV_IMG_DECLARE(eye_scare_exit_01)
LV_IMG_DECLARE(eye_scare_exit_02)
LV_IMG_DECLARE(eye_scare_exit_03)
LV_IMG_DECLARE(eye_scare_exit_04)
LV_IMG_DECLARE(eye_scare_exit_05)
LV_IMG_DECLARE(eye_scare_exit_06)
LV_IMG_DECLARE(eye_scare_exit_07)
LV_IMG_DECLARE(eye_scare_exit_08)

LV_IMG_DECLARE(eye_buxue_00)
LV_IMG_DECLARE(eye_buxue_01)
LV_IMG_DECLARE(eye_buxue_02)
LV_IMG_DECLARE(eye_buxue_03)
LV_IMG_DECLARE(eye_buxue_04)
LV_IMG_DECLARE(eye_buxue_05)
LV_IMG_DECLARE(eye_buxue_06)
LV_IMG_DECLARE(eye_buxue_07)
LV_IMG_DECLARE(eye_buxue_08)
LV_IMG_DECLARE(eye_buxue_09)
LV_IMG_DECLARE(eye_buxue_10)
LV_IMG_DECLARE(eye_buxue_12)
LV_IMG_DECLARE(eye_buxue_13)
LV_IMG_DECLARE(eye_buxue_15)
LV_IMG_DECLARE(eye_buxue_27)
LV_IMG_DECLARE(eye_buxue_28)
LV_IMG_DECLARE(eye_buxue_29)
LV_IMG_DECLARE(eye_buxue_30)
LV_IMG_DECLARE(eye_buxue_31)

LV_IMG_DECLARE(eye_anger_loop00)
LV_IMG_DECLARE(eye_anger_loop01)
LV_IMG_DECLARE(eye_anger_loop02)
LV_IMG_DECLARE(eye_anger_loop03)
LV_IMG_DECLARE(eye_anger_loop04)
LV_IMG_DECLARE(eye_anger_loop05)
LV_IMG_DECLARE(eye_anger_loop06)
LV_IMG_DECLARE(eye_anger_loop07)
LV_IMG_DECLARE(eye_anger_loop08)
LV_IMG_DECLARE(eye_anger_loop09)
LV_IMG_DECLARE(eye_anger_loop10)
LV_IMG_DECLARE(eye_anger_loop11)
LV_IMG_DECLARE(eye_anger_exit_00)
LV_IMG_DECLARE(eye_anger_exit_01)
LV_IMG_DECLARE(eye_anger_exit_02)
LV_IMG_DECLARE(eye_anger_exit_03)
LV_IMG_DECLARE(eye_anger_exit_04)
LV_IMG_DECLARE(eye_anger_exit_05)
LV_IMG_DECLARE(eye_anger_exit_06)
LV_IMG_DECLARE(eye_anger_exit_07)
LV_IMG_DECLARE(eye_anger_exit_08)

typedef struct
{
    lv_img_dsc_t **images;
    size_t size;
    int time;
    int offset_x;
    int offset_y;
} ai_anim_list_t;

typedef struct
{
    int id;
    int offset_x;
    int offset_y;
    ai_anim_list_t *list;
    void (*on_anim_complate_cb)(lv_anim_t *);
} eye_emotion_t;

// Animation state
static lv_anim_t s_img_anim;
static ai_anim_list_t *s_current_anim = NULL;
static void (*s_anim_complete_cb)(lv_anim_t *) = NULL;

static void _on_idle_complate_cb(lv_anim_t *anim);
static void _on_eye_sad_complate_cb(lv_anim_t *anim);
static void _on_eye_happy_complate_cb(lv_anim_t *anim);
static void _on_eye_scare_complate_cb(lv_anim_t *anim);
static void _on_eye_buxue_complate_cb(lv_anim_t *anim);
static void _on_eye_anger_complate_cb(lv_anim_t *anim);

// 静态表情
lv_img_dsc_t *eye_static_idle[] = {
    &eye_static_00,
    &eye_static_00,
    &eye_static_00,
    &eye_static_00,
    &eye_static_00,
    &eye_static_00,
};
lv_img_dsc_t *eye_static_once[] = {
    &eye_static_00,
    &eye_static_02,
    &eye_static_03,
    &eye_static_04,
    &eye_static_05,
    &eye_static_00,
};
lv_img_dsc_t *eye_static_twice[] = {
    &eye_static_00,
    &eye_static_02,
    &eye_static_03,
    &eye_static_04,
    &eye_static_05,
    &eye_static_00,
    &eye_static_02,
    &eye_static_03,
    &eye_static_04,
    &eye_static_05,
    &eye_static_00,
};

lv_img_dsc_t *eye_sad_loop[] = {
    &eye_sad_loop_00,
    &eye_sad_loop_01,
    &eye_sad_loop_02,
    &eye_sad_loop_03,
    &eye_sad_loop_04,
    &eye_sad_loop_05,
};

lv_img_dsc_t *eye_sad_loop1[] = {
    &eye_sad_loop_06,
    &eye_sad_loop_07,
    &eye_sad_loop_08,
    &eye_sad_loop_09,
    &eye_sad_loop_10,
    &eye_sad_loop_11,
};

lv_img_dsc_t *eye_sad_loop2[] = {
    &eye_sad_loop_12,
    &eye_sad_loop_13,
    &eye_sad_loop_14,
    &eye_sad_loop_15,
    &eye_sad_loop_16,
    &eye_sad_loop_17,
};

lv_img_dsc_t *eye_sad_loop3[] = {
    &eye_sad_loop_16,
    &eye_sad_loop_15,
    &eye_sad_loop_14,
};

lv_img_dsc_t *eye_sad_exit[] = {
    &eye_sad_exit_00,
    &eye_sad_exit_01,
    &eye_sad_exit_02,
    &eye_sad_exit_03,
    &eye_sad_exit_04,
    &eye_sad_exit_05,
    &eye_sad_exit_06,
};

lv_img_dsc_t *eye_happy_in[] = {
    &eye_happy_01,
    &eye_happy_03,
    &eye_happy_04,
    &eye_happy_05,
    &eye_happy_06,
    &eye_happy_07,
};

lv_img_dsc_t *eye_happy_loop[] = {
    // &eye_happy_08,
    &eye_happy_09,
    &eye_happy_10,
    &eye_happy_11,
};

lv_img_dsc_t *eye_happy_loop1[] = {
    &eye_happy_12,
    &eye_happy_13,
    &eye_happy_14,
    &eye_happy_15,
};

lv_img_dsc_t *eye_happy_loop2[] = {
    &eye_happy_17,
};

lv_img_dsc_t *eye_happy_loop3[] = {
    &eye_happy_15,
    &eye_happy_14,
    &eye_happy_13,
    &eye_happy_12,
    &eye_happy_11,
    &eye_happy_10,
    &eye_happy_09,
};

lv_img_dsc_t *eye_happy_exit[] = {
    &eye_happy_27,
    &eye_happy_28,
    &eye_happy_29,
    &eye_happy_30,
    &eye_happy_31,
    &eye_happy_01,
    &eye_happy_01,
};

// 350
lv_img_dsc_t *eye_scare_loop[] = {
    &eye_scare_01,
    &eye_scare_02,
    &eye_scare_03,
    &eye_scare_04,
    &eye_scare_05,
};

// 350
lv_img_dsc_t *eye_scare_loop1[] = {
    &eye_scare_05,
};

lv_img_dsc_t *eye_scare_loop2[] = {
    &eye_scare_11,
    &eye_scare_12,
    &eye_scare_13,
    &eye_scare_14,
    &eye_scare_15,
    &eye_scare_16,
};

// 350
lv_img_dsc_t *eye_scare_loop3[] = {
    &eye_scare_16,
};

lv_img_dsc_t *eye_scare_exit[] = {
    // &eye_scare_exit_00,
    &eye_scare_exit_01,
    &eye_scare_exit_02,
    &eye_scare_exit_03,
    &eye_scare_exit_04,
    &eye_scare_exit_05,
    &eye_scare_exit_06,
    &eye_scare_exit_07,
    &eye_scare_exit_08,
};

// 困惑表情
lv_img_dsc_t *eye_buxue_in[] = {
    &eye_buxue_00,
    &eye_buxue_01,
    &eye_buxue_02,
    &eye_buxue_03,
    &eye_buxue_04,
    &eye_buxue_05,
    &eye_buxue_06,
    &eye_buxue_07,
};
lv_img_dsc_t *eye_buxue_loop1[] = {
    &eye_buxue_08,
    &eye_buxue_09,
};
lv_img_dsc_t *eye_buxue_loop2[] = {
    &eye_buxue_10,
};
lv_img_dsc_t *eye_buxue_loop3[] = {
    &eye_buxue_12,
    &eye_buxue_13,
};
lv_img_dsc_t *eye_buxue_loop4[] = {
    &eye_buxue_15,
};
lv_img_dsc_t *eye_buxue_exit[] = {

    &eye_buxue_08,
    &eye_buxue_27,
    &eye_buxue_28,
    &eye_buxue_29,
    &eye_buxue_31,
    &eye_buxue_00,
};

lv_img_dsc_t *eye_anger_loop[] = {
    &eye_anger_loop00,
    &eye_anger_loop01,
    &eye_anger_loop02,
    &eye_anger_loop03,
    &eye_anger_loop04,
    &eye_anger_loop05,
};
lv_img_dsc_t *eye_anger_loop1[] = {
    &eye_anger_loop06,
    &eye_anger_loop07,
    &eye_anger_loop08,
    &eye_anger_loop09,
    &eye_anger_loop10,
    &eye_anger_loop11,
};
lv_img_dsc_t *eye_anger_loop2[] = {
    &eye_anger_loop11,
    &eye_anger_loop10,
    &eye_anger_loop09,
    &eye_anger_loop08,
    &eye_anger_loop07,
    &eye_anger_loop06,
};
lv_img_dsc_t *eye_anger_loop3[] = {
    &eye_anger_loop05,
    &eye_anger_loop04,
    &eye_anger_loop03,
    &eye_anger_loop02,
    &eye_anger_loop01,
    &eye_anger_loop00,
};

lv_img_dsc_t *eye_anger_exit[] = {
    &eye_anger_exit_00,
    &eye_anger_exit_01,
    &eye_anger_exit_02,
    &eye_anger_exit_03,
    &eye_anger_exit_04,
    &eye_anger_exit_05,
    &eye_anger_exit_06,
    &eye_anger_exit_07,
    &eye_anger_exit_08,
};

static ai_anim_list_t anim_idle_list[] = {
    {eye_static_idle, sizeof(eye_static_idle) / sizeof(lv_img_dsc_t *), 240},
    {eye_static_once, sizeof(eye_static_once) / sizeof(lv_img_dsc_t *), 220},
    {eye_static_twice, sizeof(eye_static_twice) / sizeof(lv_img_dsc_t *), 440}};

static ai_anim_list_t anim_sad_list[] = {
    {eye_sad_loop, sizeof(eye_sad_loop) / sizeof(lv_img_dsc_t *), 300},
    {eye_sad_loop1, sizeof(eye_sad_loop1) / sizeof(lv_img_dsc_t *), 300},
    {eye_sad_loop2, sizeof(eye_sad_loop2) / sizeof(lv_img_dsc_t *), 300},
    {eye_sad_loop3, sizeof(eye_sad_loop3) / sizeof(lv_img_dsc_t *), 300},
    {eye_sad_exit, sizeof(eye_sad_exit) / sizeof(lv_img_dsc_t *), 300, -3, -10}};

static ai_anim_list_t anim_happy_list[] = {
    {eye_happy_in, sizeof(eye_happy_in) / sizeof(lv_img_dsc_t *), 400},
    {eye_happy_loop, sizeof(eye_happy_loop) / sizeof(lv_img_dsc_t *), 380},
    {eye_happy_loop1, sizeof(eye_happy_loop1) / sizeof(lv_img_dsc_t *), 280},
    {eye_happy_loop2, sizeof(eye_happy_loop2) / sizeof(lv_img_dsc_t *), 200},
    {eye_happy_loop3, sizeof(eye_happy_loop3) / sizeof(lv_img_dsc_t *), 300},
    {eye_happy_exit, sizeof(eye_happy_exit) / sizeof(lv_img_dsc_t *), 380}};

static ai_anim_list_t anim_scare_list[] = {
    {eye_scare_loop, sizeof(eye_scare_loop) / sizeof(lv_img_dsc_t *), 350},
    {eye_scare_loop1, sizeof(eye_scare_loop1) / sizeof(lv_img_dsc_t *), 350},
    {eye_scare_loop2, sizeof(eye_scare_loop2) / sizeof(lv_img_dsc_t *), 350},
    {eye_scare_loop3, sizeof(eye_scare_loop3) / sizeof(lv_img_dsc_t *), 350},
    {eye_scare_exit, sizeof(eye_scare_exit) / sizeof(lv_img_dsc_t *), 350, -3, -12}};

static ai_anim_list_t anim_buxue_list[] = {
    {eye_buxue_in, sizeof(eye_buxue_in) / sizeof(lv_img_dsc_t *), 300},
    {eye_buxue_loop1, sizeof(eye_buxue_loop1) / sizeof(lv_img_dsc_t *), 100},
    {eye_buxue_loop2, sizeof(eye_buxue_loop2) / sizeof(lv_img_dsc_t *), 300},
    {eye_buxue_loop3, sizeof(eye_buxue_loop3) / sizeof(lv_img_dsc_t *), 100},
    {eye_buxue_loop4, sizeof(eye_buxue_loop4) / sizeof(lv_img_dsc_t *), 300},
    {eye_buxue_exit, sizeof(eye_buxue_exit) / sizeof(lv_img_dsc_t *), 300}};

static ai_anim_list_t anim_anger_list[] = {
    {eye_anger_loop, sizeof(eye_anger_loop) / sizeof(lv_img_dsc_t *), 300},
    {eye_anger_loop1, sizeof(eye_anger_loop1) / sizeof(lv_img_dsc_t *), 300},
    {eye_anger_loop2, sizeof(eye_anger_loop2) / sizeof(lv_img_dsc_t *), 300},
    {eye_anger_loop3, sizeof(eye_anger_loop3) / sizeof(lv_img_dsc_t *), 300},
    {eye_anger_exit, sizeof(eye_anger_exit) / sizeof(lv_img_dsc_t *), 300, -3, -16}};

static eye_emotion_t eye_idle_emotion = {
    .id = 0,
    .offset_x = -1,
    .offset_y = -15,
    .list = anim_idle_list,
    .on_anim_complate_cb = _on_idle_complate_cb};

static eye_emotion_t eye_sad_emotion = {
    .id = 1,
    .offset_x = -3,
    .offset_y = -10,
    .list = anim_sad_list,
    .on_anim_complate_cb = _on_eye_sad_complate_cb};

static eye_emotion_t eye_happy_emotion = {
    .id = 2,
    .offset_x = -3,
    .offset_y = -28,
    .list = anim_happy_list,
    .on_anim_complate_cb = _on_eye_happy_complate_cb};

static eye_emotion_t eye_scare_emotion = {
    .id = 3,
    .offset_x = 0,
    .offset_y = -6,
    .list = anim_scare_list,
    .on_anim_complate_cb = _on_eye_scare_complate_cb};

static eye_emotion_t eye_buxue_emotion = {
    .id = 4,
    .offset_x = 0,
    .offset_y = -6,
    .list = anim_buxue_list,
    .on_anim_complate_cb = _on_eye_buxue_complate_cb};

static eye_emotion_t eye_anger_emotion = {
    .id = 5,
    .offset_x = -2,
    .offset_y = -3,
    .list = anim_anger_list,
    .on_anim_complate_cb = _on_eye_anger_complate_cb};

static eye_emotion_t *s_emotion_list[] = {
    &eye_idle_emotion,
    &eye_sad_emotion,
    &eye_happy_emotion,
    &eye_scare_emotion,
    &eye_buxue_emotion,
    &eye_anger_emotion,
};

// Animation exec callback - called by LVGL animation engine
static void _anim_exec_cb(void *var, int32_t value)
{
    lv_obj_t *img = (lv_obj_t *)var;
    if (!s_current_anim || value < 0 || value >= (int32_t)s_current_anim->size)
    {
        return;
    }
    lv_img_set_src(img, s_current_anim->images[value]);
}

// Animation ready callback - called when animation completes
static void _anim_ready_cb(lv_anim_t *anim)
{
    if (s_anim_complete_cb)
    {
        s_anim_complete_cb(anim);
    }
}

static void _set_animimg_src(lv_obj_t *obj, ai_anim_list_t *anim_list, void (*on_anim_complate_cb)(lv_anim_t *))
{
    // Delete any existing animation on this object
    lv_anim_delete(obj, NULL);

    // Store animation list and callback
    s_current_anim = anim_list;
    s_anim_complete_cb = on_anim_complate_cb;

    // Set first frame immediately
    if (anim_list->size > 0)
    {
        lv_img_set_src(obj, anim_list->images[0]);
    }

    // If only one frame, call completion immediately
    if (anim_list->size <= 1)
    {
        if (on_anim_complate_cb)
        {
            on_anim_complate_cb(NULL);
        }
        return;
    }

    // Initialize animation
    lv_anim_init(&s_img_anim);
    lv_anim_set_var(&s_img_anim, obj);
    lv_anim_set_exec_cb(&s_img_anim, _anim_exec_cb);
    lv_anim_set_values(&s_img_anim, 0, anim_list->size - 1);
    lv_anim_set_duration(&s_img_anim, anim_list->time);
    lv_anim_set_repeat_count(&s_img_anim, 1);
    lv_anim_set_path_cb(&s_img_anim, lv_anim_path_step); // Step path for discrete frames

    if (on_anim_complate_cb)
    {
        lv_anim_set_completed_cb(&s_img_anim, _anim_ready_cb);
    }

    // Start animation
    lv_anim_start(&s_img_anim);
}
static void _on_idle_complate_cb(lv_anim_t *anim)
{
    static uint8_t idle_time = 0;
    static uint8_t last_index = 0;
    uint8_t index = (uint8_t)(rand() % 3);
    if (last_index != 0)
    {
        index = 0;
    }

    if (last_index == 0 && idle_time < 8)
    {
        idle_time++;
        index = 0;
        if (idle_time >= 8)
        {
            index = (uint8_t)(rand() % 3);
            if (index == 0)
            {
                index = 1;
            }
        }
    }
    if (index != 0)
    {
        idle_time = 0;
    }

    if (emotion != eye_idle_emotion.id)
    {
        index = 0;
        idle_time = 0;
        last_index = 0;
        if (last_index == 0)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
    }

    last_index = index;
    _set_animimg_src(s_animimg, &eye_idle_emotion.list[index], eye_idle_emotion.on_anim_complate_cb);
}

static void _on_eye_sad_complate_cb(lv_anim_t *anim)
{
    static uint8_t last_index = 0;
    uint8_t index = (last_index + 1) % 4;
    if (emotion != eye_sad_emotion.id)
    {
        index = 4;
        if (emotion != 0 || last_index == 4)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
        else
        {
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, eye_sad_emotion.list[index].offset_x, eye_sad_emotion.list[index].offset_y);
        }
    }
    last_index = index;
    _set_animimg_src(s_animimg, &eye_sad_emotion.list[index], eye_sad_emotion.on_anim_complate_cb);
}

static void _on_eye_happy_complate_cb(lv_anim_t *anim)
{
    static uint8_t last_index = 0;
    uint8_t index = 1;
    // last_index = (last_index+1)%3;
    if (emotion != eye_happy_emotion.id)
    {
        index = 5;
        if (emotion != 0 || last_index == 5)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
    }
    else
    {
        index = last_index + 1;
        if (index >= 5)
        {
            index = 1;
        }
    }

    last_index = index;
    _set_animimg_src(s_animimg, &eye_happy_emotion.list[index], eye_happy_emotion.on_anim_complate_cb);
}

static void _on_eye_scare_complate_cb(lv_anim_t *anim)
{
    static uint8_t last_index = 0;
    uint8_t index = (last_index + 1) % 3;
    if (emotion != eye_scare_emotion.id)
    {
        index = 4;
        if (emotion != 0 || last_index == 4)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
        else
        {
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, eye_scare_emotion.list[index].offset_x, eye_scare_emotion.list[index].offset_y);
        }
    }
    last_index = index;
    _set_animimg_src(s_animimg, &eye_scare_emotion.list[index], eye_scare_emotion.on_anim_complate_cb);
}

static void _on_eye_buxue_complate_cb(lv_anim_t *anim)
{
    static uint8_t last_index = 0;
    uint8_t index = 1;
    // last_index = (last_index+1)%3;
    if (emotion != eye_buxue_emotion.id)
    {
        index = 5;
        if (emotion != 0 || last_index == 5)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
    }
    else
    {
        switch (last_index)
        {
        case 0:
            index = 1;
            break;
        case 1:
            index = 2;
            break;
        case 2:
            index = 3;
            break;
        case 3:
            index = 4;
            break;
        case 4:
            index = 1;
            break;
        default:
            break;
        }
    }
    last_index = index;
    _set_animimg_src(s_animimg, &eye_buxue_emotion.list[index], eye_buxue_emotion.on_anim_complate_cb);
}

static void _on_eye_anger_complate_cb(lv_anim_t *anim)
{
    static uint8_t last_index = 0;
    uint8_t index = (last_index + 1) % 4;
    if (emotion != eye_anger_emotion.id)
    {
        index = 4;
        if (emotion != 0 || last_index == 4)
        {
            last_index = 0;
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, s_emotion_list[emotion]->offset_x, s_emotion_list[emotion]->offset_y);
            _set_animimg_src(s_animimg, &s_emotion_list[emotion]->list[0], s_emotion_list[emotion]->on_anim_complate_cb);
            return;
        }
        else
        {
            lv_obj_align(s_animimg, LV_ALIGN_CENTER, eye_anger_emotion.list[index].offset_x, eye_anger_emotion.list[index].offset_y);
        }
    }
    last_index = index;
    _set_animimg_src(s_animimg, &eye_anger_emotion.list[index], eye_anger_emotion.on_anim_complate_cb);
}

void eye_set_emotion(int emo)
{
    printf("index:%d\r\n", emo);
    emotion = emo;
}

lv_obj_t *create_emotion_eye(lv_obj_t *parent)
{
    lv_obj_t *eye_gif = lv_img_create(parent);
    s_animimg = eye_gif;
    emotion = 0;
    lv_obj_align(s_animimg, LV_ALIGN_CENTER, eye_idle_emotion.offset_x, eye_idle_emotion.offset_y);
    _set_animimg_src(s_animimg, &eye_idle_emotion.list[0], eye_idle_emotion.on_anim_complate_cb);

    return eye_gif;
}