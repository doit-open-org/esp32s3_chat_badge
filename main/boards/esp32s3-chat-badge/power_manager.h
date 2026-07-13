#pragma once
#include <vector>
#include <functional>

#include <esp_timer.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#define TAG "power_manager"
class PowerManager {
private:
    esp_timer_handle_t timer_handle_;
    std::function<void(bool)> on_charging_status_changed_;
    std::function<void(bool)> on_low_battery_status_changed_;

    gpio_num_t charging_pin_ = GPIO_NUM_NC;
    std::vector<uint16_t> adc_values_;
    uint32_t battery_level_ = 100;
    bool is_charging_ = false;
    bool is_low_battery_ = false;
    int ticks_ = 0;
    const int kBatteryAdcInterval = 60;
    const int kBatteryAdcDataCount = 3;
    const int kLowBatteryLevel = 20;

    adc_channel_t adc_channel_ = ADC_CHANNEL_0;

    int voltage_max_ = 0;
    int voltage_min_ = 0;
    
    adc_oneshot_unit_handle_t adc_handle_;

    bool do_calibration_ = false;
    adc_cali_handle_t adc_cali_handle_ = NULL;


    bool AdcGpio2channel(gpio_num_t adc_gpio, adc_unit_t &unit_id, adc_channel_t &channel){
#if (defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2))
        if(adc_gpio > GPIO_NUM_4){
            return false;
        }
        unit_id = ADC_UNIT_1;
        channel = (adc_channel_t)adc_gpio;
        return true;
#elif (defined(CONFIG_IDF_TARGET_ESP32S3))
        if(adc_gpio == 0 || adc_gpio > 20){
            return false;
        }
        if(adc_gpio <= GPIO_NUM_10){
            unit_id = ADC_UNIT_1;
            channel = (adc_channel_t)(adc_gpio - 1);
        }else{
            unit_id = ADC_UNIT_2;
            channel = (adc_channel_t)(adc_gpio - 11);
        }
        return true;
#else
        return false;
#endif

    }

    void CheckBatteryStatus() {
        // Get charging status
        bool new_charging_status = gpio_get_level(charging_pin_) == 1;
        if (new_charging_status != is_charging_) {
            is_charging_ = new_charging_status;
            if (on_charging_status_changed_) {
                on_charging_status_changed_(is_charging_);
            }
            ReadBatteryAdcData();
            return;
        }

        // 如果电池电量数据不足，则读取电池电量数据
        if (adc_values_.size() < kBatteryAdcDataCount) {
            ReadBatteryAdcData();
            return;
        }

        // 如果电池电量数据充足，则每 kBatteryAdcInterval 个 tick 读取一次电池电量数据
        ticks_++;
        if (ticks_ % kBatteryAdcInterval == 0) {
            ReadBatteryAdcData();
        }
    }

    void ReadBatteryAdcData() {
        int adc_value;
        int voltage = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, adc_channel_, &adc_value));
        if (do_calibration_) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle_, adc_value, &voltage));
            ESP_LOGI(TAG, "ADC Cali Voltage: %d mV",  voltage);
            adc_value = voltage;
        }

        // 将 ADC 值添加到队列中
        adc_values_.push_back(adc_value);
        if (adc_values_.size() > kBatteryAdcDataCount) {
            adc_values_.erase(adc_values_.begin());
        }
        uint32_t average_adc = 0;
        for (auto value : adc_values_) {
            average_adc += value;
        }
        average_adc /= adc_values_.size();

        // 定义电池电量区间
        struct {
            uint16_t adc;
            uint8_t level;
        } levels[] = {
            {2350 - 80*5, 0},
            {2350 - 80*4, 20},
            {2350 - 80*3, 40},
            {2350 - 80*2, 60},
            {2350 - 80*1, 80},
            {2350 - 80*0, 100}
        };

        int diff = (voltage_max_ - voltage_min_)/5;
        for (size_t i = 0; i < 6; i++){
            levels[i].adc = voltage_min_ + i*diff;
        }
        
        // 低于最低值时
        if (average_adc < levels[0].adc) {
            battery_level_ = 0;
        }
        // 高于最高值时
        else if (average_adc >= levels[5].adc) {
            battery_level_ = 100;
        } else {
            // 线性插值计算中间值
            for (int i = 0; i < 5; i++) {
                if (average_adc >= levels[i].adc && average_adc < levels[i+1].adc) {
                    float ratio = static_cast<float>(average_adc - levels[i].adc) / (levels[i+1].adc - levels[i].adc);
                    battery_level_ = levels[i].level + ratio * (levels[i+1].level - levels[i].level);
                    break;
                }
            }
        }

        // Check low battery status
        if (adc_values_.size() >= kBatteryAdcDataCount) {
            bool new_low_battery_status = battery_level_ <= kLowBatteryLevel;
            if (new_low_battery_status != is_low_battery_) {
                is_low_battery_ = new_low_battery_status;
                if (on_low_battery_status_changed_) {
                    on_low_battery_status_changed_(is_low_battery_);
                }
            }
        }

        ESP_LOGI("PowerManager", "ADC value: %d average: %ld level: %ld", adc_value, average_adc, battery_level_);
    }

    bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
    {
        adc_cali_handle_t handle = NULL;
        esp_err_t ret = ESP_FAIL;
        bool calibrated = false;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
            adc_cali_curve_fitting_config_t cali_config = {
                .unit_id = unit,
                .chan = channel,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

    #if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
            adc_cali_line_fitting_config_t cali_config = {
                .unit_id = unit,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

        *out_handle = handle;
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Calibration Success");
        } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
            ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
        } else {
            ESP_LOGE(TAG, "Invalid arg or no memory");
        }

        return calibrated;
    }

public:
    PowerManager(gpio_num_t charging_pin, gpio_num_t adc_pin, int voltage_max, int voltage_min, adc_oneshot_unit_handle_t adc_handle = nullptr) : charging_pin_(charging_pin), voltage_max_(voltage_max), voltage_min_(voltage_min), adc_handle_(adc_handle){
        adc_unit_t unit_id = ADC_UNIT_1;

        // 初始化充电引脚
        if(charging_pin_ != GPIO_NUM_NC){
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pin_bit_mask = (1ULL << charging_pin_);
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     
            gpio_config(&io_conf);
        }

        if(AdcGpio2channel(adc_pin, unit_id, adc_channel_)){
            // 创建电池电量检查定时器
            esp_timer_create_args_t timer_args = {
                .callback = [](void* arg) {
                    PowerManager* self = static_cast<PowerManager*>(arg);
                    self->CheckBatteryStatus();
                },
                .arg = this,
                .dispatch_method = ESP_TIMER_TASK,
                .name = "battery_check_timer",
                .skip_unhandled_events = true,
            };
            ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
            ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, 1000000));

            if(adc_handle_ == NULL){
                // 初始化 ADC
                adc_oneshot_unit_init_cfg_t init_config = {
                    .ulp_mode = ADC_ULP_MODE_DISABLE,
                };
                init_config.unit_id = unit_id;
                ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));
            }

            adc_oneshot_chan_cfg_t chan_config = {
                .atten = ADC_ATTEN_DB_12,
                .bitwidth = ADC_BITWIDTH_12,
            };
            ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, adc_channel_, &chan_config));

            do_calibration_ = example_adc_calibration_init(unit_id, adc_channel_, ADC_ATTEN_DB_12, &adc_cali_handle_);
        }
    }

    ~PowerManager() {
        if (timer_handle_) {
            esp_timer_stop(timer_handle_);
            esp_timer_delete(timer_handle_);
        }
        if (adc_handle_) {
            adc_oneshot_del_unit(adc_handle_);
        }
    }

    bool IsCharging() {
        // 如果电量已经满了，则不再显示充电中
        if (battery_level_ == 100) {
            return false;
        }
        return is_charging_;
    }

    bool IsDischarging() {
        // 没有区分充电和放电，所以直接返回相反状态
        return !is_charging_;
    }

    uint8_t GetBatteryLevel() {
        return battery_level_;
    }

    void OnLowBatteryStatusChanged(std::function<void(bool)> callback) {
        on_low_battery_status_changed_ = callback;
    }

    void OnChargingStatusChanged(std::function<void(bool)> callback) {
        on_charging_status_changed_ = callback;
    }
};
