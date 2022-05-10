/**
 * @file selftest_frame_esp.cpp
 */

#include "selftest_frame_esp.hpp"
#include "selftest_esp_type.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "marlin_client.hpp"
#include "resource.h"
#include <cstring>

static constexpr size_t col_0 = WizardDefaults::MarginLeft;

SelftestFrameESP::SelftestFrameESP(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameWithRadio>(parent, ph, data)
    , text(this, Rect16(col_0, WizardDefaults::row_0, WizardDefaults::X_space, WizardDefaults::txt_h * 10), is_multiline::yes) {
    text.SetAlignment(Align_t::LeftTop());
    change();
}

void SelftestFrameESP::change() {

    const char *txt = "MISSING";

    //texts
    switch (phase_current) {
    case PhasesSelftest::ESP_instructions:
        txt = N_("Make sure USB drive with config file is connected.\n\nContinue to upload settings to the printer.");
        break;
    case PhasesSelftest::ESP_USB_not_inserted:
        txt = N_("USB drive not detected! Insert USB drive first!");
        break;
    case PhasesSelftest::ESP_ask_gen:
        txt = N_("Generate Wi-Fi credentials?");
        break;
    case PhasesSelftest::ESP_ask_gen_overwrite:
        txt = N_("Config detected on the USB drive. Overwrite current file?");
        break;
    case PhasesSelftest::ESP_makefile_failed:
        txt = N_("Creating the file failed! Check the USB drive!");
        break;
    case PhasesSelftest::ESP_eject_USB:
        txt = N_("Success!\nRemove the drive.\nEdit the file in PC.");
        break;
    case PhasesSelftest::ESP_insert_USB:
        txt = N_("Insert USB drive with valid INI file.");
        break;
    case PhasesSelftest::ESP_invalid:
        txt = N_("Loading the file failed! Check the USB drive!");
        break;
    case PhasesSelftest::ESP_uploading_config:
        txt = N_("Uploading config to the printer.\n\nPlease wait.");
        break;
    case PhasesSelftest::ESP_enabling_WIFI:
        txt = N_("Success!\nPlease wait until the connection is established.\n\nIf nothing happens after 5-8 minutes, check & load your credentials again.");
        break;
    case PhasesSelftest::ESP_uploaded:
        txt = N_("Connection successfully established! Wi-Fi is now ready for use.");
        break;
    default:
        break;
    }

    text.SetText(_(txt));
};
