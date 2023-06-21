/**
 * @file MItem_loadcell.hpp
 * @brief loadcell menu items
 */
#pragma once
#include "WindowMenuItems.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "i18n.h"
#include "sensor_data_buffer.hpp"

class MI_TEST_LOADCELL : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Loadcell");

public:
    MI_TEST_LOADCELL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_LOADCELL_SCALE : public WiSpinInt {
    constexpr static const char *const label = "Loadcell Scale";

public:
    MI_LOADCELL_SCALE();
    virtual void OnClick() override;
};

class MI_INFO_LOADCELL : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
    static constexpr const char *const label = N_("Loadcell Value");

public:
    MI_INFO_LOADCELL();
};
