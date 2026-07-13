#ifndef _CUSTOM_OTA_H
#define _CUSTOM_OTA_H

#include <functional>
#include <string>
#include <map>

#include "ota.h"

class CustomOta : public Ota {
public:
    CustomOta();
    ~CustomOta();

    virtual bool CheckVersion() override;
    virtual std::string GetCheckVersionUrl() override;
    bool forced() { return forced_; };
private:
    bool forced_ = false;
};

#endif // _OTA_H
