#!/usr/bin/env python3
"""Compile the real material manager on the host and exercise temporary files."""

from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
PLATFORM_STUB = r"""
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct lv_obj_t lv_obj_t;
typedef void *EventGroupHandle_t;
typedef uint32_t EventBits_t;
#define LV_OBJ_FLAG_HIDDEN 1
#define ESP_LOGI(tag, ...) ((void)(tag))
#define ESP_LOGW(tag, ...) ((void)(tag))
#define ESP_LOGE(tag, ...) ((void)(tag))
static inline EventGroupHandle_t xEventGroupCreate(void) { return (void *)1; }
static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t g, EventBits_t b) { return b; }
static inline EventBits_t xEventGroupGetBits(EventGroupHandle_t g) { return 0; }
static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t g, EventBits_t b) { return 0; }
static inline bool lvgl_port_lock(int timeout) { return true; }
static inline void lvgl_port_unlock(void) {}
static inline void lv_obj_add_flag(lv_obj_t *obj, int flag) {}
static inline lv_obj_t *doit_ui_get_show_lv_obj(void) { return 0; }
static inline void doit_ui_init(lv_obj_t *obj, uint16_t w, uint16_t h) {}
static inline void doit_decode_init(void) {}
static inline void doit_img_decode(const char *path) {}
static inline void doit_vpg_player_start(const char *path) {}
static const char *test_background_path;
static bool test_background_cleared;
static inline bool chat_background_is_current(const char *path) {
    return test_background_path && strcmp(path, test_background_path) == 0;
}
static inline void chat_background_clear(void) { test_background_cleared = true; }
"""

HARNESS = r"""
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char test_root[PATH_MAX];
static const char *fail_open_path;
static bool usb_mode;
bool zw_sdcard_is_usb_mode(void) { return usb_mode; }

static const char *map_path(const char *path, char *buffer) {
    if (strncmp(path, "/sdcard", 7) == 0 && (path[7] == '/' || path[7] == '\0')) {
        int n = snprintf(buffer, PATH_MAX, "%s%s", test_root, path + 7);
        assert(n > 0 && n < PATH_MAX);
        return buffer;
    }
    return path;
}
static int mapped_stat(const char *path, struct stat *st) {
    char buffer[PATH_MAX]; return stat(map_path(path, buffer), st);
}
static FILE *mapped_fopen(const char *path, const char *mode) {
    if (fail_open_path && strcmp(path, fail_open_path) == 0) {
        errno = EIO; return NULL;
    }
    char buffer[PATH_MAX]; return fopen(map_path(path, buffer), mode);
}
static DIR *mapped_opendir(const char *path) {
    char buffer[PATH_MAX]; return opendir(map_path(path, buffer));
}
static int mapped_mkdir(const char *path, mode_t mode) {
    char buffer[PATH_MAX]; return mkdir(map_path(path, buffer), mode);
}
static int mapped_remove(const char *path) {
    char buffer[PATH_MAX]; return remove(map_path(path, buffer));
}
static int mapped_unlink(const char *path) {
    char buffer[PATH_MAX]; return unlink(map_path(path, buffer));
}
static int mapped_rename(const char *from, const char *to) {
    char a[PATH_MAX], b[PATH_MAX];
    const char *dest = map_path(to, b);
    // FatFs refuses to rename over an existing destination.
    if (access(dest, F_OK) == 0) { errno = EEXIST; return -1; }
    return rename(map_path(from, a), dest);
}
#define stat(...) mapped_stat(__VA_ARGS__)
#define fopen(...) mapped_fopen(__VA_ARGS__)
#define opendir(...) mapped_opendir(__VA_ARGS__)
#define mkdir(...) mapped_mkdir(__VA_ARGS__)
#define remove(...) mapped_remove(__VA_ARGS__)
#define unlink(...) mapped_unlink(__VA_ARGS__)
#define rename(...) mapped_rename(__VA_ARGS__)
#include "doit_file.c"

static const char *names[] = {
    "01.jpg", "1.jpg", "02.jpeg", "03.JPG", "05.jpg",
    "01_copy.jpg", "0.jpg", "4294967296.jpg", "07.jpg"
};
static void write_image(const char *name, unsigned char marker, bool valid) {
    char path[256]; snprintf(path, sizeof(path), "/sdcard/photo/%s", name);
    unsigned char data[32] = {0xff, 0xd8, 0xff, 0xe1};
    if (!valid) memset(data, 0, sizeof(data));
    data[4] = marker;
    FILE *f = fopen(path, "wb"); assert(f);
    assert(fwrite(data, 1, sizeof(data), f) == sizeof(data));
    assert(fclose(f) == 0);
}
static void assert_original_files(bool first_deleted) {
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        char path[256]; snprintf(path, sizeof(path), "/sdcard/photo/%s", names[i]);
        FILE *f = fopen(path, "rb");
        if (first_deleted && i == 0) { assert(!f); continue; }
        assert(f);
        unsigned char data[32]; assert(fread(data, 1, sizeof(data), f) == sizeof(data));
        assert(data[4] == i + 1); assert(fclose(f) == 0);
    }
    // Scanning must never manufacture or rename a file to this path.
    struct stat st; assert(stat("/sdcard/photo/02.jpg", &st) != 0);
}
static void assert_index_readable(void) {
    material_group_t *g = &g_groups[MATERIAL_TYPE_IMG];
    for (uint32_t i = 0; i < g->count; i++) {
        FILE *f = fopen(g->files[i].filepath, "rb"); assert(f); fclose(f);
        for (uint32_t j = i + 1; j < g->count; j++)
            assert(strcmp(g->files[i].filepath, g->files[j].filepath) != 0);
    }
}
int main(int argc, char **argv) {
    assert(argc == 2); snprintf(test_root, sizeof(test_root), "%s", argv[1]);
    assert(mkdir("/sdcard/photo", 0700) == 0);
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        write_image(names[i], i + 1, i != 8);

    // Repeated boots/rescans preserve real names, distinct contents and selection.
    for (int repeat = 0; repeat < 5; repeat++) {
        scan_directory(MATERIAL_TYPE_IMG);
        assert(doit_file_get_count(MATERIAL_TYPE_IMG) == 5);
        assert_original_files(false); assert_index_readable();
        doit_file_set_type(MATERIAL_TYPE_IMG);
        assert(strcmp(get_show_dir(), "/sdcard/photo/01.jpg") == 0);
        // Invalid 07.jpg remains on disk and its name stays reserved for uploads.
        assert(doit_file_get_next_download_index(MATERIAL_TYPE_IMG) == 8);
    }
    // A transient read failure excludes a file without deleting it.
    fail_open_path = "/sdcard/photo/05.jpg";
    scan_directory(MATERIAL_TYPE_IMG);
    assert(doit_file_get_count(MATERIAL_TYPE_IMG) == 4);
    fail_open_path = NULL;
    assert_original_files(false);
    scan_directory(MATERIAL_TYPE_IMG);
    assert(doit_file_get_count(MATERIAL_TYPE_IMG) == 5);

    usb_mode = true;
    scan_directory(MATERIAL_TYPE_IMG);
    assert(doit_file_delete_current().err_code == CL_OPERT_FAIL);
    assert_original_files(false);
    usb_mode = false;
    test_background_path = "/sdcard/photo/05.jpg";
    assert(doit_file_delete_current().err_code == CL_OPRET_SUCCESS);
    assert(!test_background_cleared);
    assert_original_files(true); assert_index_readable();
    assert(strcmp(get_show_dir(), "/sdcard/photo/1.jpg") == 0);
    assert(doit_file_get_next_download_index(MATERIAL_TYPE_IMG) == 8);

    write_image("08.jpg", 100, true);
    assert(doit_file_add_downloaded("/sdcard/photo/08.jpg", MATERIAL_TYPE_IMG));
    assert(strcmp(get_show_dir(), "/sdcard/photo/08.jpg") == 0);
    assert(doit_file_get_next_download_index(MATERIAL_TYPE_IMG) == 9);
    scan_directory(MATERIAL_TYPE_IMG);
    assert(doit_file_get_count(MATERIAL_TYPE_IMG) == 5);
    assert_index_readable(); assert_original_files(true);

    uint32_t idx; char ext[8];
    assert(!parse_filename("01_copy.jpg", &idx, ext));
    assert(!parse_filename("+1.jpg", &idx, ext));
    assert(!parse_filename(" 1.jpg", &idx, ext));
    assert(!parse_filename("0.jpg", &idx, ext));
    assert(!parse_filename("4294967296.jpg", &idx, ext));
    assert(parse_filename("4294967295.jpg", &idx, ext) && idx == UINT32_MAX);
    g_groups[MATERIAL_TYPE_IMG].max_index = UINT32_MAX;
    assert(doit_file_get_next_download_index(MATERIAL_TYPE_IMG) == 0);
    puts("Material regression checks passed: real filenames, repeated scans, read failure, deletion, uploads.");
    return 0;
}
"""


def main():
    compiler = shutil.which("cc")
    if not compiler:
        raise SystemExit("A host C compiler (cc) is required.")
    component = ROOT / "components/doit_min_program_upload"
    with tempfile.TemporaryDirectory(prefix="badge-material-test-") as directory:
        work = Path(directory)
        stubs = work / "stubs"
        stubs.mkdir()
        (stubs / "platform.h").write_text(PLATFORM_STUB)
        for name in (
            "lvgl.h", "esp_jpeg_common.h", "esp_jpeg_dec.h", "esp_log.h",
            "chat_background.h", "doit_decode.h", "doit_ui.h", "esp_lvgl_port.h",
            "img_decode.h", "vpg_decode.h", "freertos/FreeRTOS.h", "freertos/event_groups.h",
        ):
            path = stubs / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "platform.h"\n')
        harness = work / "regression.c"
        harness.write_text(HARNESS)
        executable = work / "regression"
        subprocess.run([
            compiler, "-std=gnu11", "-Werror=implicit-function-declaration",
            "-I", str(stubs), "-I", str(component / "include/file"),
            "-I", str(component / "file"), str(harness), "-o", str(executable),
        ], check=True)
        sdcard = work / "sdcard"
        sdcard.mkdir()
        subprocess.run([str(executable), str(sdcard)], check=True)


if __name__ == "__main__":
    main()
