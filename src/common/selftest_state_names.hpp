/**
 * @file selftest_state_names.hpp
 * @brief names attached to selftest states
 */

#pragma once

#include "client_response.hpp"

constexpr const char *get_selftest_state_name(PhasesSelftest state) {

    switch (state) {
    case PhasesSelftest::_none:
        return "_none";
    case PhasesSelftest::WizardPrologue_ask_run:
        return "WizardPrologue_ask_run";
    case PhasesSelftest::WizardPrologue_ask_run_dev:
        return "WizardPrologue_ask_run_dev";
    case PhasesSelftest::WizardPrologue_info:
        return "WizardPrologue_info";
    case PhasesSelftest::WizardPrologue_info_detailed:
        return "WizardPrologue_info_detailed";
    case PhasesSelftest::ESP_instructions:
        return "ESP_instructions";
    case PhasesSelftest::ESP_USB_not_inserted:
        return "ESP_USB_not_inserted";
    case PhasesSelftest::ESP_ask_gen:
        return "ESP_ask_gen";
    case PhasesSelftest::ESP_ask_gen_overwrite:
        return "ESP_ask_gen_overwrite";
    case PhasesSelftest::ESP_makefile_failed:
        return "ESP_makefile_failed";
    case PhasesSelftest::ESP_eject_USB:
        return "ESP_eject_USB";
    case PhasesSelftest::ESP_insert_USB:
        return "ESP_insert_USB";
    case PhasesSelftest::ESP_invalid:
        return "ESP_invalid";
    case PhasesSelftest::ESP_uploading_config:
        return "ESP_uploading_config";
    case PhasesSelftest::ESP_enabling_WIFI:
        return "ESP_enabling_WIFI";
    case PhasesSelftest::ESP_uploaded:
        return "ESP_uploaded";
    case PhasesSelftest::ESP_progress_info:
        return "ESP_progress_info";
    case PhasesSelftest::ESP_progress_upload:
        return "ESP_progress_upload";
    case PhasesSelftest::ESP_progress_passed:
        return "ESP_progress_passed";
    case PhasesSelftest::ESP_progress_failed:
        return "ESP_progress_failed";
    case PhasesSelftest::ESP_qr_instructions_flash:
        return "ESP_qr_instructions_flash";
    case PhasesSelftest::ESP_qr_instructions:
        return "ESP_qr_instructions";
    case PhasesSelftest::Fans:
        return "Fans";
    case PhasesSelftest::Axis:
        return "Axis";
    case PhasesSelftest::Heaters:
        return "Heaters";
    case PhasesSelftest::FirstLayer_mbl:
        return "FirstLayer_mbl";
    case PhasesSelftest::FirstLayer_print:
        return "FirstLayer_print";
    case PhasesSelftest::FirstLayer_filament_known_and_not_unsensed:
        return "FirstLayer_filament_known_and_not_unsensed";
    case PhasesSelftest::FirstLayer_filament_not_known_or_unsensed:
        return "FirstLayer_filament_not_known_or_unsensed";
    case PhasesSelftest::FirstLayer_calib:
        return "FirstLayer_calib";
    case PhasesSelftest::FirstLayer_use_val:
        return "FirstLayer_use_val";
    case PhasesSelftest::FirstLayer_start_print:
        return "FirstLayer_start_print";
    case PhasesSelftest::FirstLayer_reprint:
        return "FirstLayer_reprint";
    case PhasesSelftest::FirstLayer_clean_sheet:
        return "FirstLayer_clean_sheet";
    case PhasesSelftest::FirstLayer_failed:
        return "FirstLayer_failed";
    case PhasesSelftest::Result:
        return "Result";
    case PhasesSelftest::WizardEpilogue_ok:
        return "WizardEpilogue_ok";
    case PhasesSelftest::WizardEpilogue_nok:
        return "WizardEpilogue_nok";
    }
    return "ERROR_not_a_selftest_state";
}
