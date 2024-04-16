/**
 * @file MItem_loadcell.cpp
 */
#include "MItem_loadcell.hpp"
#include "printer_selftest.hpp"
#include "marlin_client.hpp"
#include "ScreenSelftest.hpp"
#include "ScreenHandler.hpp"
#include "loadcell.hpp"
#include "WindowMenuSpin.hpp"
#include <config_store/store_instance.hpp>

/*****************************************************************************/
// MI_TEST_LOADCELL
MI_TEST_LOADCELL::MI_TEST_LOADCELL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_LOADCELL::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_client::test_start(stmLoadcell);
}

/*****************************************************************************/
// MI_LOADCELL_SCALE

static constexpr NumericInputConfig loadcell_scale_spin_config = {
    .min_value = 5,
    .max_value = 30,
};

MI_LOADCELL_SCALE::MI_LOADCELL_SCALE()
    : WiSpin((int)(config_store().loadcell_scale.get() * 1000), loadcell_scale_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_LOADCELL_SCALE::OnClick() {
    float scale = value() / 1000;
    loadcell.SetScale(scale);
    config_store().loadcell_scale.set(scale);
}

/*****************************************************************************/
// MI_INFO_LOADCELL
MI_INFO_LOADCELL::MI_INFO_LOADCELL()
    : WI_FORMATABLE_LABEL_t<SensorData::Value>(
        _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, {}, [&](char *buffer) {
            if (value.is_valid()) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f", (double)value.get_float());
            } else {
                if (value.is_enabled()) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {
}
