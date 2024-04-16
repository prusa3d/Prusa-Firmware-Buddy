#include "esp_frame_text.hpp"

#include "selftest_esp_type.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "marlin_client.hpp"
#include <cstring>

static constexpr size_t col_0 = WizardDefaults::MarginLeft;

ESPFrameText::ESPFrameText(window_t *parent, PhasesESP ph, fsm::PhaseData data)
    : AddSuperWindow<ESPFrame>(parent, ph, data)
    , text(this, Rect16(col_0, WizardDefaults::row_0, WizardDefaults::X_space, WizardDefaults::txt_h * 10), is_multiline::yes) {
    text.SetAlignment(Align_t::LeftTop());
    change();
}

void ESPFrameText::change() {

    const char *txt = "MISSING";

    // texts
    switch (phase_current) {
    case PhasesESP::ESP_instructions:
        txt = N_("Make sure USB drive with config file is connected.\n\nContinue to upload settings to the printer.");
        break;
    case PhasesESP::ESP_USB_not_inserted:
        txt = N_("USB drive not detected! Insert USB drive first!");
        break;
    case PhasesESP::ESP_ask_gen:
        txt = N_("Generate Wi-Fi credentials?");
        break;
    case PhasesESP::ESP_ask_gen_overwrite:
        txt = N_("Config detected on the USB drive. Overwrite current file?");
        break;
    case PhasesESP::ESP_makefile_failed:
        txt = N_("Creating the file failed! Check the USB drive!");
        break;
    case PhasesESP::ESP_eject_USB:
        txt = N_("Success!\nRemove the drive.\nEdit the file in PC.");
        break;
    case PhasesESP::ESP_insert_USB:
        txt = N_("Insert USB drive with valid INI file.");
        break;
    case PhasesESP::ESP_invalid:
        txt = N_("Loading the file failed! Check the USB drive!");
        break;
    case PhasesESP::ESP_uploading_config:
        txt = N_("Uploading config to the printer.\n\nPlease wait.");
        break;
    case PhasesESP::ESP_asking_credentials_delete:
        txt = N_("Credentials loaded.\n\nDelete credentials file? (Recommended)");
        break;
    case PhasesESP::ESP_enabling_WIFI:
        txt = N_("Attempting to connect.\n\nYou may continue using printer. The Wi-Fi icon will appear in the status bar once connected.\n\nIf nothing happens after a few minutes, check & reload the credentials.");
        break;
    default:
        break;
    }

    text.SetText(_(txt));
};
