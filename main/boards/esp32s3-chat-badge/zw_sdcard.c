#include "zw_sdcard.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "hal/gpio_types.h"
#include "lvgl.h"
#include "portmacro.h"
#include "sd_protocol_types.h"
#include "sdmmc_cmd.h"
#include "vfs_fat_internal.h"
#include <stdbool.h>
#include "tusb_msc_storage.h"
#include "tusb.h"
#include "tusb_option.h"
#include "tinyusb_msc.h"
#include "tinyusb.h"
#include <inttypes.h>

#define TAG "zw_sdcard"
#define USB_DETECT_GPIO CHARGE_DETECT
// 全局变量
sdmmc_card_t *card = NULL;
static bool s_usb_mode = false;
static bool s_switching_mode = false;
static bool s_lvgl_fs_registered = false;
static bool s_tinyusb_installed = false;
static bool s_msc_created = false;
static SemaphoreHandle_t s_mode_mutex = NULL;
static lv_fs_drv_t fs_drv;

// USB检测相关
static bool s_usb_detect_initialized = false;
static esp_timer_handle_t s_usb_debounce_timer = NULL;
static void (*s_usb_unplug_callback)(void) = NULL;

// ============== LVGL 文件系统回调 ==============

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
  (void)drv;

  // USB模式下阻止文件访问
  if (s_usb_mode)
  {
    ESP_LOGW(TAG, "fs_open blocked: USB mode active, path: %s", path);
    return NULL;
  }

  ESP_LOGI(TAG, "fs_open called with path: %s", path);

  const char *modes;
  if (mode == LV_FS_MODE_WR)
  {
    modes = "wb";
  }
  else if (mode == LV_FS_MODE_RD)
  {
    modes = "rb";
  }
  else
  {
    return NULL;
  }

  char full_path[256];
  snprintf(full_path, sizeof(full_path), "%s%s", ZW_SDCARD_MOUNTPOINT, path);

  FILE *fp = fopen(full_path, modes);
  if (fp)
  {
    ESP_LOGI(TAG, "fs_open: File opened successfully");
  }
  else
  {
    ESP_LOGE(TAG, "fs_open: Failed to open file: %s", full_path);
  }

  return fp;
}

static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
  (void)drv;
  fclose(file_p);
  return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf,
                           uint32_t btr, uint32_t *br)
{
  (void)drv;
  long pos = ftell(file_p);
  *br = fread(buf, 1, btr, file_p);
  if (pos == 0)
  {
    ESP_LOGI(TAG, "fs_read first chunk: btr=%" PRIu32 " br=%" PRIu32, btr, *br);
  }
  return LV_FS_RES_OK;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos,
                           lv_fs_whence_t whence)
{
  (void)drv;

  int w;
  switch (whence)
  {
  case LV_FS_SEEK_SET:
    w = SEEK_SET;
    break;
  case LV_FS_SEEK_CUR:
    w = SEEK_CUR;
    break;
  case LV_FS_SEEK_END:
    w = SEEK_END;
    break;
  default:
    return LV_FS_RES_INV_PARAM;
  }

  fseek(file_p, pos, w);
  return LV_FS_RES_OK;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
  (void)drv;
  *pos_p = ftell(file_p);
  return LV_FS_RES_OK;
}

static void fs_init(void)
{
  if (s_lvgl_fs_registered)
  {
    ESP_LOGW(TAG, "LVGL filesystem driver already registered");
    return;
  }

  lv_fs_drv_init(&fs_drv);
  fs_drv.letter = 'P';
  fs_drv.open_cb = fs_open;
  fs_drv.close_cb = fs_close;
  fs_drv.read_cb = fs_read;
  fs_drv.seek_cb = fs_seek;
  fs_drv.tell_cb = fs_tell;
  lv_fs_drv_register(&fs_drv);
  s_lvgl_fs_registered = true;
  ESP_LOGI(TAG, "LVGL filesystem driver registered");
}

// ============== SD卡挂载函数 ==============

static esp_err_t zw_sdcard_mount(void)
{
  esp_err_t ret;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = ZW_SDCARD_MAX_OPEN_FILES,
      .allocation_unit_size = ZW_SDCARD_ALLOCATE_UNIT_SIZE,
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0;
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4;
  slot_config.clk = SD_CLK;
  slot_config.cmd = SD_CMD;
  slot_config.d0 = SD_D0;
  slot_config.d1 = SD_D1;
  slot_config.d2 = SD_D2;
  slot_config.d3 = SD_D3;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(ZW_SDCARD_MOUNTPOINT, &host, &slot_config,
                                &mount_config, &card);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
    }
    return ret;
  }
  ESP_LOGI(TAG, "Filesystem mounted");
  uint64_t card_bytes = (uint64_t)card->csd.capacity * card->csd.sector_size;
  unsigned int card_mib = (unsigned int)(card_bytes / (1024ULL * 1024ULL));
  ESP_LOGI(TAG, "SD capacity: %u MiB (%.2f GiB)", card_mib,
           (double)card_mib / 1024.0);
  uint64_t fs_total_bytes = 0;
  uint64_t fs_free_bytes = 0;
  esp_err_t info_ret = esp_vfs_fat_info(ZW_SDCARD_MOUNTPOINT, &fs_total_bytes, &fs_free_bytes);
  if (info_ret == ESP_OK)
  {
    unsigned int fs_total_mib = (unsigned int)(fs_total_bytes / (1024ULL * 1024ULL));
    unsigned int fs_free_mib = (unsigned int)(fs_free_bytes / (1024ULL * 1024ULL));
    ESP_LOGI(TAG, "SD free: %u/%u MiB (%.2f/%.2f GiB)", fs_free_mib, fs_total_mib,
             (double)fs_free_mib / 1024.0, (double)fs_total_mib / 1024.0);
  }
  else
  {
    ESP_LOGW(TAG, "Failed to get SD free space (%s).", esp_err_to_name(info_ret));
  }

  sdmmc_card_print_info(stdout, card);
  fs_init();

  return ret;
}

static esp_err_t zw_sdcard_unmount(void)
{
  return esp_vfs_fat_sdcard_unmount(ZW_SDCARD_MOUNTPOINT, card);
}

// ============== USB MSC 模式切换 ==============

void zw_sdcard_usb_msc_enable(void)
{
  // 初始化互斥锁
  if (s_mode_mutex == NULL)
  {
    s_mode_mutex = xSemaphoreCreateMutex();
    if (s_mode_mutex == NULL)
    {
      ESP_LOGE(TAG, "Failed to create mode mutex");
      return;
    }
  }

  // 获取互斥锁
  if (xSemaphoreTake(s_mode_mutex, pdMS_TO_TICKS(5000)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to acquire mode mutex (timeout)");
    return;
  }

  if (s_usb_mode)
  {
    ESP_LOGW(TAG, "USB MSC already enabled");
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  if (s_switching_mode)
  {
    ESP_LOGW(TAG, "USB MSC is switching, ignore enable request");
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  s_switching_mode = true;

  if (!zw_sdcard_is_usb_connected())
  {
    ESP_LOGW(TAG, "USB not connected, cannot enable USB MSC");
    s_switching_mode = false;
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  if (card == NULL)
  {
    ESP_LOGE(TAG, "SD card is NULL, cannot enable USB MSC");
    s_switching_mode = false;
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  esp_err_t ret;

  // 安装TinyUSB驱动
  if (!s_tinyusb_installed)
  {
    tinyusb_config_t tusb_cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = {
            .skip_setup = false,
            .self_powered = false,
            .vbus_monitor_io = 0,
        },
        .task = {
            .size = 4096,
            .priority = 5,
            .xCoreID = 0,
        },
        .descriptor = {
            .device = NULL,
            .qualifier = NULL,
            .string = NULL,
            .string_count = 0,
            .full_speed_config = NULL,
            .high_speed_config = NULL,
        },
        .event_cb = NULL,
        .event_arg = NULL,
    };

    ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(ret));
      s_switching_mode = false;
      xSemaphoreGive(s_mode_mutex);
      return;
    }
    s_tinyusb_installed = true;
  }

  //创建MSC存储
  if (!s_msc_created)
  {
    const tinyusb_msc_storage_config_t msc_cfg = {
        .medium.card = card,
    };
    ret = tinyusb_msc_new_storage_sdmmc(&msc_cfg, NULL);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to create MSC storage: %s", esp_err_to_name(ret));
      s_switching_mode = false;
      xSemaphoreGive(s_mode_mutex);
      return;
    }
    s_msc_created = true;
  }

  // 连接USB

  tud_connect();
  vTaskDelay(pdMS_TO_TICKS(20));

  s_usb_mode = true;
  s_switching_mode = false;
  xSemaphoreGive(s_mode_mutex);
}

void zw_sdcard_usb_msc_disable(void)
{
  // 初始化互斥锁
  if (s_mode_mutex == NULL)
  {
    s_mode_mutex = xSemaphoreCreateMutex();
    if (s_mode_mutex == NULL)
    {
      ESP_LOGE(TAG, "Failed to create mode mutex");
      return;
    }
  }

  // 获取互斥锁
  if (xSemaphoreTake(s_mode_mutex, pdMS_TO_TICKS(5000)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to acquire mode mutex (timeout)");
    return;
  }

  if (!s_usb_mode)
  {
    ESP_LOGW(TAG, "USB MSC already disabled");
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  if (s_switching_mode)
  {
    ESP_LOGW(TAG, "USB MSC is switching, ignore disable request");
    xSemaphoreGive(s_mode_mutex);
    return;
  }

  s_switching_mode = true;
  // 断开USB连接
  tud_disconnect();
  vTaskDelay(pdMS_TO_TICKS(20));

  s_usb_mode = false;
  s_switching_mode = false;
  xSemaphoreGive(s_mode_mutex);
}

bool zw_sdcard_is_usb_mode(void)
{
  return s_usb_mode;
}

// ============== 公共接口 ==============

esp_err_t zw_sdcard_init(void)
{
  esp_err_t ret = zw_sdcard_mount();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "sdcard mount failed.");
    return ret;
  }
  return ret;
}

void zw_sdcard_deinit(void)
{
  zw_sdcard_unmount();
  ESP_LOGI(TAG, "Card unmounted");
}

// ============== USB 检测功能 ==============

// 防抖动定时器回调
static void usb_debounce_timer_callback(void *arg)
{
  // 读取当前USB连接状态
  bool is_connected = gpio_get_level(USB_DETECT_GPIO) == 1;

  // 如果USB拔出且当前处于USB模式，触发回调
  if (!is_connected && s_usb_mode && s_usb_unplug_callback)
  {
    ESP_LOGI(TAG, "USB unplugged while in USB mode, triggering callback");
    s_usb_unplug_callback();
  }
}

// GPIO中断处理函数
static void IRAM_ATTR usb_gpio_isr_handler(void *arg)
{
  // 启动防抖动定时器（50ms）
  if (s_usb_debounce_timer)
  {
    esp_timer_stop(s_usb_debounce_timer);
    esp_timer_start_once(s_usb_debounce_timer, 50000); // 50ms
  }
}

void zw_sdcard_usb_detect_init(void)
{
  if (s_usb_detect_initialized)
  {
    return;
  }
  // 创建防抖动定时器
  esp_timer_create_args_t timer_args = {
      .callback = usb_debounce_timer_callback,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "usb_debounce",
  };
  esp_timer_create(&timer_args, &s_usb_debounce_timer);

  // 设置GPIO中断类型
  gpio_set_intr_type(USB_DETECT_GPIO, GPIO_INTR_ANYEDGE);

  // 安装GPIO中断服务
  gpio_install_isr_service(0);
  gpio_isr_handler_add(USB_DETECT_GPIO, usb_gpio_isr_handler, NULL);

  s_usb_detect_initialized = true;
  ESP_LOGI(TAG, "USB detect initialized on GPIO%d", USB_DETECT_GPIO);
}

bool zw_sdcard_is_usb_connected(void)
{
  return gpio_get_level(USB_DETECT_GPIO) == 1;
}

void zw_sdcard_set_usb_unplug_callback(void (*callback)(void))
{
  s_usb_unplug_callback = callback;
}
