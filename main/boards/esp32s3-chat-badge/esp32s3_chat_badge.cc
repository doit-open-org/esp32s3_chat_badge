#include "application.h"
#include "config.h"
#include "wifi_board.h"
#include <cstddef>
#include <cstring>
#include <inttypes.h>
#include <vector>
#include <esp_log.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_vendor.h>
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_jd9855.h"
#include "esp_lcd_touch_cst816d.h"
#include "esp_lvgl_port_touch.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "display/lcd_display.h"
#include "display/lv_display.h"
#include "audio/codecs/vb6824_audio_codec.h"
#include "button.h"
#include "power_manager.h"
#include "power_save_timer.h"
#include "assets/lang_config.h"
#include "zw_sdcard.h"
#include "mcp_server.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "display/ui_tool.h"
#include <esp_psram.h>
#include <esp_lv_decoder.h>

extern "C"
{
#include "display/cl_ui.h"
#include "doit_file.h"
#include "doit_ble.h"
#include "display/chat_background.h"
}
#define TAG "CustomBoard"

extern const lv_image_dsc_t logo;

static void ShowBootLogo()
{
  if (!lvgl_port_lock(pdMS_TO_TICKS(1000)))
  {
    ESP_LOGW(TAG, "ShowBootLogo: failed to lock LVGL");
    return;
  }

  lv_obj_t *screen = lv_screen_active();
  if (screen != nullptr)
  {
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *img = lv_image_create(screen);
    lv_image_set_src(img, &logo);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
  }

  lvgl_port_unlock();
}

// U盘模式状态
static bool g_usb_mode_active = false;

class CustomBoard;
static CustomBoard *g_board = nullptr;

// 前向声明
static void exit_usb_mode();

// USB拔出回调函数
static void on_usb_unplugged()
{
  if (g_usb_mode_active)
  {
    ESP_LOGI(TAG, "USB unplugged, auto-exiting USB mode");
    exit_usb_mode();
  }
}

// U盘模式切换函数
static void enter_usb_mode()
{
  if (g_usb_mode_active)
  {
    ESP_LOGW(TAG, "Already in USB mode");
    return;
  }

  // 检查USB是否已连接
  if (!zw_sdcard_is_usb_connected())
  {
    ESP_LOGW(TAG, "USB not connected, cannot enter USB mode");
    return;
  }

  // 设置USB拔出回调
  zw_sdcard_set_usb_unplug_callback(on_usb_unplugged);

  //先切换到U盘模式页面
  page_change("usb_mode");
  vTaskDelay(pdMS_TO_TICKS(100));

  // 启用USB MSC
  zw_sdcard_usb_msc_enable();

  // 检查是否成功进入USB模式
  if (zw_sdcard_is_usb_mode())
  {
    g_usb_mode_active = true;
    ESP_LOGI(TAG, "USB mode enabled successfully");
    Application::GetInstance().PlaySound(Lang::Sounds::OGG_USB_ENTER);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to enable USB mode, returning to home");
    zw_sdcard_set_usb_unplug_callback(NULL); // 清除回调
    page_change("home");
  }
}

static void exit_usb_mode()
{
  if (!g_usb_mode_active)
  {
    ESP_LOGW(TAG, "Not in USB mode");
    return;
  }

  // 清除USB拔出回调
  zw_sdcard_set_usb_unplug_callback(NULL);
  
  zw_sdcard_usb_msc_disable();
  vTaskDelay(pdMS_TO_TICKS(100));

  g_usb_mode_active = false;
  Application::GetInstance().PlaySound(Lang::Sounds::OGG_USB_EXIT);

  // 刷新文件索引
  doit_file_rescan_materials();

  //切换回主页
  page_change("home");
}

static void toggle_usb_mode()
{
  if (g_usb_mode_active)
  {
    exit_usb_mode();
  }
  else
  {
    enter_usb_mode();
  }
}
class CustomSpiLcdDisplay : public LcdDisplay
{
public:
  CustomSpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                      int width, int height, int offset_x, int offset_y,
                      bool mirror_x, bool mirror_y, bool swap_xy)
      : LcdDisplay(panel_io, panel, width, height)
  {
    // Initialize SPI LCD display (copied from SpiLcdDisplay constructor)

    // Draw white screen
    std::vector<uint16_t> buffer(width, 0xFFFF);
    for (int y = 0; y < height; y++)
    {
      esp_lcd_panel_draw_bitmap(panel_, 0, y, width, y + 1, buffer.data());
    }

    // Set the display to on
    ESP_LOGI(TAG, "Turning display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

#if CONFIG_SPIRAM
    // lv image cache, currently only PNG is supported
    size_t psram_size_mb = esp_psram_get_size() / 1024 / 1024;
    if (psram_size_mb >= 8)
    {
      lv_image_cache_resize(2 * 1024 * 1024, true);
      ESP_LOGI(TAG, "Use 2MB of PSRAM for image cache");
    }
    else if (psram_size_mb >= 2)
    {
      lv_image_cache_resize(512 * 1024, true);
      ESP_LOGI(TAG, "Use 512KB of PSRAM for image cache");
    }
#endif

    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Adding LCD display");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .control_handle = nullptr,
        .enable_tearing_effect = true,
        .te_gpio = QSPI_PIN_NUM_LCD_TE,
        .tear_effect_level_edge = 1,
        .tearing_timeout_ms = 500,
        .buffer_size = static_cast<uint32_t>(width * height),//15
        .double_buffer = true,
        .trans_size = 0,
        .hres = static_cast<uint32_t>(width),
        .vres = static_cast<uint32_t>(height),
        .monochrome = false,
        .rotation =
            {
                .swap_xy = swap_xy,
                .mirror_x = mirror_x,
                .mirror_y = mirror_y,
            },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags =
            {
                .buff_dma = 1,
                .buff_spiram = 1,
                .sw_rotate = 0,
                .swap_bytes = 1,
                .full_refresh = 0,
                .direct_mode = 0,
            },
    };

    display_ = lvgl_port_add_disp(&display_cfg);

    // Initialize decoder
    esp_lv_decoder_handle_t decoder_handle = NULL;
    esp_lv_decoder_init(&decoder_handle);

    if (display_ == nullptr)
    {
      ESP_LOGE(TAG, "Failed to add display");
      return;
    }

    if (offset_x != 0 || offset_y != 0)
    {
      lv_display_set_offset(display_, offset_x, offset_y);
    }

    ShowBootLogo();
  }

  void SetEmotion(const char *emotion) override
  {
    LcdDisplay::SetEmotion(emotion);
    if (emotion)
    {
      cl_ui_send_event(CL_UI_EVENT_CHAT_EMOJI, emotion);
    }
  }

  void SetChatMessage(const char *role, const char *content) override
  {
    LcdDisplay::SetChatMessage(role, content);
    if (content)
    {
      cl_ui_send_event(CL_UI_EVENT_CHAT_MSG, content);
    }
  }
};

class CustomBoard : public WifiBoard
{
private:
  Button boot_button_;
  LcdDisplay *display_;
  esp_lcd_panel_io_handle_t lcd_panel_io_ = nullptr;
  PowerSaveTimer *power_save_timer_;
  PowerManager *power_manager_ = nullptr;
  i2c_master_bus_handle_t touch_i2c_bus_;
  VbAduioCodec *audio_codec_ = nullptr;

  void InitializePowerManager()
  {
    power_manager_ = new PowerManager(CHARGE_DETECT, VBAT_DETECT, 688, 492);
  }
  void InitializePowerSaveTimer()
  {
    uint32_t auto_shutdown_time = ui_tool_get_sleep_time();
#define TEST_AUTO_SHUTDOWN 0

    int shutdown_seconds;
#if TEST_AUTO_SHUTDOWN
    shutdown_seconds = 100;
#else
    shutdown_seconds = (auto_shutdown_time > 0) ? (int)auto_shutdown_time : -1;
#endif
    power_save_timer_ = new PowerSaveTimer(-1, -1, shutdown_seconds);

    // 设置睡眠模式回调
    // power_save_timer_->OnEnterSleepMode([this]()
    //                                     { });

    // power_save_timer_->OnExitSleepMode([this]()
    //                                    {
    //     GetBacklight()->RestoreBrightness(); });
    power_save_timer_->OnShutdownRequest([this]()
                                         { esp_restart(); });
    power_save_timer_->SetEnabled(true);
  }

  void InitializeFileSystem()
  {
    doit_file_system_init();
  }

  void InitializeChatBackground()
  {
    chat_background_init();
  }

public:
  void InitializeButtons()
  {
    boot_button_.OnClick([this]()
                         {
                           // U盘模式下不处理单击
                           if (g_usb_mode_active) return;
                           
                           cl_button_t btn = {
                               .scan_cnt = 1,
                               .click_cnt = 1,
                               .id = CL_UI_KEY_MODE,
                               .event = CL_BTN_CLICK};
                           ui_key_msg(&btn, nullptr); });
    boot_button_.OnDoubleClick([this]()
                               {
                                 // U盘模式下不处理双击
                                 if (g_usb_mode_active) return;
                                 
                                 cl_button_t btn = {
                                     .scan_cnt = 1,
                                     .click_cnt = 2,
                                     .id = CL_UI_KEY_MODE,
                                     .event = CL_BTN_DOUBLE_CLICK};
                                 ui_key_msg(&btn, nullptr); });

    // 长按切换U盘模式
    boot_button_.OnLongPress([this]()
                             {
      ESP_LOGI(TAG, "Long press detected");
      
      // 发送长按事件到UI层
      cl_button_t btn = {
          .scan_cnt = 1,
          .click_cnt = 1,
          .id = CL_UI_KEY_MODE,
          .event = CL_BTN_LONG_START};
      ui_key_msg(&btn, nullptr);
      
      // 切换U盘模式
      toggle_usb_mode(); });

    // 长按重复点击3次重置WiFi配置（仅在非U盘模式下）
    boot_button_.OnPressRepeat([this](uint16_t count)
                               {
      if (g_usb_mode_active) return;
      
      if (count >= 3) {
        ResetWifiConfiguration();
        ESP_LOGI(TAG, "Boot button pressed 3 times, entering WiFi config mode");
      } });

    boot_button_.OnPressDown([this]() {

    });
  }

private:
  void InitializeTouchI2c()
  {
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = TP_SDA,
        .scl_io_num = TP_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags =
            {
                .enable_internal_pullup = 1,
            },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &touch_i2c_bus_));
    if (TP_RST != GPIO_NUM_NC)
    {
      gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);
      gpio_set_level(TP_RST, 1);
      vTaskDelay(pdMS_TO_TICKS(10));
      gpio_set_level(TP_RST, 0);
      vTaskDelay(pdMS_TO_TICKS(10));
      gpio_set_level(TP_RST, 1);
      vTaskDelay(pdMS_TO_TICKS(110));
    }
  }

  void InitializeSpi()
  {
    const spi_bus_config_t bus_config = {
        .data0_io_num = QSPI_PIN_NUM_LCD_DATA0,
        .data1_io_num = QSPI_PIN_NUM_LCD_DATA1,
        .sclk_io_num = QSPI_PIN_NUM_LCD_PCLK,
        .data2_io_num = QSPI_PIN_NUM_LCD_DATA2,
        .data3_io_num = QSPI_PIN_NUM_LCD_DATA3,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(
        spi_bus_initialize(QSPI_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
  }

  void Initializejd9855Display()
  {
    esp_lcd_panel_handle_t panel = nullptr;

    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = QSPI_PIN_NUM_LCD_CS,
        .dc_gpio_num = GPIO_NUM_NC,
        .spi_mode = 0,
        .pclk_hz = 80 * 1000 * 1000,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,

        .flags =
            {
                .quad_mode = true,
            },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)QSPI_LCD_HOST, &io_config, &lcd_panel_io_));

    jd9855_vendor_config_t vendor_config = {
        .flags = {.use_qspi_interface = 1},
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = QSPI_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_lcd_new_panel_jd9855(lcd_panel_io_, &panel_config, &panel);

    esp_lcd_panel_reset(panel);
    esp_lcd_panel_disp_on_off(panel, true);
    esp_lcd_panel_init(panel);

    display_ = new CustomSpiLcdDisplay(lcd_panel_io_, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                       DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                       DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    // lv_obj_t *screen = lv_screen_active();
    // if (screen)
    // {
    //   lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);//
    //   lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);//
    // }
  }
  static void TouchInterruptCallback(esp_lcd_touch_handle_t tp)
  {
    if (!tp || !tp->config.user_data)
      return;

    auto board = static_cast<CustomBoard *>(tp->config.user_data);
    if (!board || !board->power_save_timer_)
      return;

    if (board->power_save_timer_->IsInSleepMode())
    {
      static uint32_t last_wakeup = 0;
      uint32_t now = esp_timer_get_time() / 1000;

      if (now - last_wakeup > 500)
      {
        board->power_save_timer_->WakeUp();
        last_wakeup = now;
      }
    }
  }

  void InitializeTouch()
  {
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_CST816D_ADDRESS,
        .on_color_trans_done = 0,
        .user_ctx = 0,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 0,
        .flags =
            {
                .dc_low_on_data = 0,
                .disable_control_phase = 1,
            },
        .scl_speed_hz = 400000};
    esp_lcd_panel_io_handle_t touch_panel_io;
    esp_lcd_new_panel_io_i2c(touch_i2c_bus_, &io_config, &touch_panel_io);

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = DISPLAY_WIDTH,
        .y_max = DISPLAY_HEIGHT,
        .rst_gpio_num = TP_RST,
        .int_gpio_num = GPIO_NUM_NC,
        .levels =
            {
                .reset = 0,
                .interrupt = 0,
            },
        .flags =
            {
                .swap_xy = false,
                .mirror_x = false,
                .mirror_y = false,
            },
        .interrupt_callback = TouchInterruptCallback,
        .user_data = this,
    };

    esp_lcd_touch_handle_t tp;
    esp_lcd_touch_new_i2c_cst816d(touch_panel_io, &tp_cfg, &tp);
    lvgl_port_touch_cfg_t tp_port_cfg = {
        .disp = lv_display_get_default(),
        .handle = tp,
        .scale = {1, 1},
    };
    lv_indev_t *touch_indev = lvgl_port_add_touch(&tp_port_cfg);
    lv_indev_set_user_data(touch_indev, this);

    lv_indev_add_event_cb(touch_indev, [](lv_event_t *e)
                          {
      if (lv_event_get_code(e) != LV_EVENT_PRESSING) return;
      
      auto indev = static_cast<lv_indev_t*>(lv_event_get_target(e));
      auto board = static_cast<CustomBoard*>(lv_indev_get_user_data(indev));
      
      if (!board || !board->power_save_timer_) return;
      
      if (board->power_save_timer_->IsInSleepMode()) {
        static uint32_t last_wakeup = 0;
        uint32_t now = esp_timer_get_time() / 1000;
        
        if (now - last_wakeup > 500) {
          board->power_save_timer_->WakeUp();
          last_wakeup = now;
        }
      } }, LV_EVENT_PRESSING, nullptr);
  }
  void InitializeXiaozhiUI()
  {
    if (!lv_is_initialized())
    {
      ESP_LOGE(TAG, "LVGL is not initialized!");
      return;
    }

    lv_display_t *disp = lv_display_get_default();
    if (!disp)
    {
      ESP_LOGE(TAG, "No default display found!");
      return;
    }
    static lv_timer_t *init_timer = lv_timer_create([](lv_timer_t *timer)
                                                    {
        lv_timer_del(timer);
        ui_init(); }, 100, nullptr);
  }

  void InitializeMcp()
  {
    McpServer::GetInstance().AddTool(
        "self.get_current_version", "Gets the current version.", PropertyList(),
        [](const PropertyList &properties) -> ReturnValue
        {
          auto ota = Application::GetInstance().GetCustomOta();
          ota.CheckVersion();
          auto version = ota.GetCurrentVersion();
          return "{\"version\": \"" + version + "\"}";
        });

    McpServer::GetInstance().AddTool(
        "self.get_new_version", "Gets the new version.", PropertyList(),
        [](const PropertyList &properties) -> ReturnValue
        {
          auto ota = Application::GetInstance().GetCustomOta();
          ota.CheckVersion();
          auto new_version = ota.GetFirmwareVersion();
          return "{\"new_version\": \"" + new_version + "\"}";
        });
    McpServer::GetInstance().AddTool(
        "self.upgrade",
        "Starts the OTA upgrade process. This will check for updates and "
        "perform the upgrade if a new version is available.",
        PropertyList(), [](const PropertyList &properties) -> ReturnValue
        {
          Application::GetInstance().StartCheckNewVersionForCustom();
          return true; });
  }

public:
  CustomBoard() : boot_button_(BOOT_BUTTON_GPIO, false, (uint16_t)400, (uint16_t)0, false)
  {
    g_board = this;

    InitializePowerManager();
    InitializePowerSaveTimer();
    InitializeTouchI2c();
    InitializeSpi();
    Initializejd9855Display();
    InitializeXiaozhiUI();
    InitializeTouch();
    GetBacklight()->RestoreBrightness();
    zw_sdcard_init();
    zw_sdcard_usb_detect_init();
    InitializeFileSystem();
    InitializeChatBackground();

    InitializeButtons();
    InitializeMcp();
    audio_codec_ = new VbAduioCodec(VB6824_RX, VB6824_TX);
    audio_codec_->OnWakeUp([this](const std::string &command)
                           {
      if (command == std::string(vb6824_get_wakeup_word())){
          if(Application::GetInstance().GetDeviceState() != kDeviceStateListening){
#ifdef CONFIG_LANGUAGE_ZH_CN
              Application::GetInstance().WakeWordInvoke("你好");
#else
              Application::GetInstance().WakeWordInvoke("");
#endif
          }
      }else if (command == "开始配网"){
          ResetWifiConfiguration();
      } });
    InitializeAiPageEvents();
  }

  void InitializeAiPageEvents()
  {
    DeviceStateEventManager::GetInstance().RegisterStateChangeCallback(
        [](DeviceState previous_state, DeviceState current_state)
        {
          cl_ui_status_code_t status;
          switch (current_state)
          {
          case kDeviceStateIdle:
            status = CL_UI_STA_IDLE;
            break;
          case kDeviceStateConnecting:
            status = CL_UI_STA_CONNECTTING;
            break;
          case kDeviceStateListening:
            status = CL_UI_STA_LISTENING;
            break;
          case kDeviceStateSpeaking:
            status = CL_UI_STA_SPEAKING;
            break;
          default:
            return;
          }
          cl_ui_send_event(CL_UI_EVENT_CHAT_STATUS, &status);
        });
  }

  virtual AudioCodec *GetAudioCodec() override
  {
    return audio_codec_;
  }

  virtual Display *GetDisplay() override { return display_; }

  virtual Backlight *GetBacklight() override
  {
    static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN,
                                  DISPLAY_BACKLIGHT_OUTPUT_INVERT);
    backlight.SetBrightness(100);
    return &backlight;
  }

  virtual bool GetBatteryLevel(int &level, bool &charging, bool &discharging) override
  {
    if (power_manager_)
    {
      level = power_manager_->GetBatteryLevel();
      charging = power_manager_->IsCharging();
      discharging = power_manager_->IsDischarging();
      return true;
    }
    return false;
  }

  virtual void SetPowerSaveMode(bool enabled) override
  {
    if (!enabled)
    {
      power_save_timer_->WakeUp();
    }
    WifiBoard::SetPowerSaveMode(enabled);
  }
};
DECLARE_BOARD(CustomBoard);
