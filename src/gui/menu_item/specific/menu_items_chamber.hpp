#pragma once

#include <WindowItemTempLabel.hpp>
#include <WindowMenuSpin.hpp>

class MI_CHAMBER_TARGET_TEMP : public WiSpin {
public:
    MI_CHAMBER_TARGET_TEMP(const char *label = nullptr);
    virtual void OnClick() override;
};

class MI_CHAMBER_TEMP : public WI_TEMP_LABEL_t {
public:
    MI_CHAMBER_TEMP(const char *label = nullptr);
    void Loop() override;

private:
    std::optional<uint32_t> last_update_ms_;
};
