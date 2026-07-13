#include "sdkconfig.h"

#ifdef CONFIG_USE_CUSTOM_OTA

#include "iot/thing.h"
#include "board.h"
#include "audio_codec.h"

#include <driver/gpio.h>
#include <esp_log.h>

#include "application.h"

#define TAG "CustomOTA"

namespace iot {


class CustomOTA : public Thing {

public:
    CustomOTA() : Thing("CustomOTA", "升级服务，可查询版本和升级，升级需要先查询当前版本和最新版本，当最新版本不等于当前版本才能升级") {
        
        properties_.AddStringProperty("version", "当前版本", [this]() -> std::string {
            auto& app = Application::GetInstance();
            return app.GetCustomOta().GetCurrentVersion();
        });

        properties_.AddStringProperty("new_version", "最新版本", [this]() -> std::string {
            auto& app = Application::GetInstance();
            return app.GetCustomOta().GetFirmwareVersion();
        });

        methods_.AddMethod("Upgrade", "升级", ParameterList(), [this](const ParameterList& parameters) {
            auto& app = Application::GetInstance();
            app.StartCheckNewVersionForCustom();
        });
    }
};

} // namespace iot

DECLARE_THING(CustomOTA);

#endif