// firstlay.cpp
#include "i18n.h"
#include "gui.hpp"
#include "firstlay.hpp"
#include "filament_sensor.hpp"
#include "filament.hpp"
#include "window_dlg_preheat.hpp"
#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
#include "eeprom.h"
#include "DialogHandler.hpp"
#include <algorithm>         // std::max
#include "screen_wizard.hpp" // ChangeStartState

enum {
    FKNOWN = 0x01,      //filament is known
    F_NOTSENSED = 0x02, //filament is not in sensor
};

WizardState_t StateFnc_FIRSTLAY_FILAMENT_ASK() {
    uint8_t filament = 0;
    filament |= Filaments::CurrentIndex() != filament_t::NONE ? FKNOWN : 0;
    filament |= FS_instance().Get() == fsensor_t::NoFilament ? F_NOTSENSED : 0;

    size_t def_bt = filament == (FKNOWN | F_NOTSENSED) ? 1 : 0; //default button

    // 4 posible states
    // 0 !FKNOWN !F_NOTSENSED
    // 1 FKNOWN !F_NOTSENSED
    // 2 !FKNOWN F_NOTSENSED
    // 3 FKNOWN F_NOTSENSED
    switch (filament) {
    case FKNOWN: { //known and not "unsensed" - do not allow load
        const PhaseResponses responses = { Response::Next, Response::Unload, Response::_none, Response::_none };
        static const char en_text[] = N_("To calibrate with currently loaded filament, press NEXT. To change filament, press UNLOAD.");
        string_view_utf8 translatedText = _(en_text);
        switch (MsgBox(translatedText, responses, def_bt)) {
        case Response::Next:
            return WizardState_t::FIRSTLAY_MSBX_CALIB;
        case Response::Unload:
            return WizardState_t::FIRSTLAY_FILAMENT_UNLOAD;
        default:
            return WizardState_t::FIRSTLAY_FILAMENT_ASK_PREHEAT;
        }
    }
    case FKNOWN | F_NOTSENSED: //allow load, prepick UNLOAD, force ask preheat
    case F_NOTSENSED:          //allow load, prepick LOAD, force ask preheat
    case 0:                    //filament is not known but is sensed == most likely same as F_NOTSENSED, but user inserted filament into sensor
    default: {
        //cannot use CONTINUE button, string is too long
        const PhaseResponses responses = { Response::Next, Response::Load, Response::Unload, Response::_none };
        static const char en_text[] = N_("To calibrate with currently loaded filament, press NEXT. To load filament, press LOAD. To change filament, press UNLOAD.");
        string_view_utf8 translatedText = _(en_text);
        switch (MsgBox(translatedText, responses, def_bt)) {
        case Response::Next:
            return WizardState_t::FIRSTLAY_FILAMENT_ASK_PREHEAT;
        case Response::Load:
            return WizardState_t::FIRSTLAY_FILAMENT_LOAD;
        case Response::Unload:
            return WizardState_t::FIRSTLAY_FILAMENT_UNLOAD;
        default:
            //should not happen
            return WizardState_t::FIRSTLAY_FILAMENT_ASK_PREHEAT;
        }
    }
    }
}

WizardState_t StateFnc_FIRSTLAY_FILAMENT_ASK_PREHEAT() {
    PreheatStatus::DialogBlocking(PreheatMode::None, RetAndCool_t::Neither); //TODO header

    //Filaments::Set(gui_dlg_preheat_forced(_("Select Filament Type")));
    return WizardState_t::FIRSTLAY_MSBX_CALIB;
}

WizardState_t StateFnc_FIRSTLAY_FILAMENT_LOAD() {
    auto ret = PreheatStatus::DialogBlocking(PreheatMode::Load, RetAndCool_t::Neither);

    switch (ret) {
    case PreheatStatus::Result::DoneHasFilament:
        return WizardState_t::FIRSTLAY_MSBX_CALIB;
    case PreheatStatus::Result::Aborted:
        return WizardState_t::FIRSTLAY_FILAMENT_ASK;
    default:
        return WizardState_t::FIRSTLAY_MSBX_CALIB;
    }
}

WizardState_t StateFnc_FIRSTLAY_FILAMENT_UNLOAD() {
    auto ret = PreheatStatus::DialogBlocking(PreheatMode::Unload, RetAndCool_t::Neither);
    switch (ret) {
    case PreheatStatus::Result::DoneNoFilament:
    case PreheatStatus::Result::Aborted:
        return WizardState_t::FIRSTLAY_FILAMENT_ASK;
    default:
        return WizardState_t::FIRSTLAY_MSBX_CALIB;
    }
}

WizardState_t StateFnc_FIRSTLAY_MSBX_CALIB() {
    static const char en_text[] = N_("Now, let's calibrate the distance between the tip of the nozzle and the print sheet.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

WizardState_t StateFnc_FIRSTLAY_MSBX_USEVAL() {
    //show dialog only when values are not equal
    float diff = marlin_vars()->z_offset - z_offset_def;
    if ((diff <= -z_offset_step) || (diff >= z_offset_step)) {
        char buff[21 * 9];
        {
            char fmt[21 * 9];
            // c=21 r=9
            static const char fmt2Translate[] = N_("Do you want to use the current value?\nCurrent: %0.3f.\nDefault: %0.3f.\nClick NO to use the default value (recommended)");
            _(fmt2Translate).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
            snprintf(buff, sizeof(buff) / sizeof(char), fmt, (double)marlin_vars()->z_offset, (double)z_offset_def);
        }
        // this MakeRAM is safe - buff is allocated in RAM for the lifetime of MsgBox
        if (MsgBox(string_view_utf8::MakeRAM((const uint8_t *)buff), Responses_YesNo) == Response::No) {
            marlin_set_z_offset(z_offset_def);
            eeprom_set_var(EEVAR_ZOFFSET, variant8_flt(z_offset_def));
        }
    }

    return WizardState_t::next;
}

WizardState_t StateFnc_FIRSTLAY_MSBX_START_PRINT() {
    //static const char en_text[] = N_("Observe the pattern and turn the knob to adjust the nozzle height in real time. Extruded plastic must stick to the print surface.");
    static const char en_text[] = N_("In the next step, use the knob to adjust the nozzle height. Check the pictures in the handbook for reference.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::next;
}

//cannot add more gcodes, gcode queue is small
//and it would block dialog opening
//checking marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE))->gqueue and calling gui_loop() does not help
WizardState_t StateFnc_FIRSTLAY_PRINT() {
    DialogHandler::Open(ClientFSM::FirstLayer, 0); //open screen now, it would auto open later (on G26)

    const int temp_nozzle_preheat = int(Filaments::PreheatTemp);
    const int temp_nozzle = std::max(int(marlin_vars()->display_nozzle), int(Filaments::Current().nozzle));
    const int temp_bed = std::max(int(marlin_vars()->target_bed), int(Filaments::Current().heatbed));

    marlin_gcode("M73 P0 R0");                                             // reset progress
    marlin_gcode_printf("M104 S%d D%d", temp_nozzle_preheat, temp_nozzle); // nozzle target
    marlin_gcode_printf("M140 S%d", temp_bed);                             // bed target
    marlin_gcode_printf("M109 R%d", temp_nozzle_preheat);                  // Set target temperature, wait even if cooling
    marlin_gcode_printf("M190 R%d", temp_bed);                             // Set target temperature, wait even if cooling
    marlin_gcode("G28");                                                   // autohome
    marlin_gcode("G29");                                                   // mbl
    marlin_gcode_printf("M104 S%d", temp_nozzle);                          // set displayed temperature
    marlin_gcode_printf("M109 S%d", temp_nozzle);                          // wait for displayed temperature
    marlin_gcode("G26");                                                   // firstlay

    WizardState_t ret = WizardState_t::FIRSTLAY_MSBX_REPEAT_PRINT;
    ScreenWizard::ChangeStartState(ret); //marlin_gcode("G26"); will close wizard screen, need to save reopen state
    return ret;
}

WizardState_t StateFnc_FIRSTLAY_MSBX_REPEAT_PRINT() {
    static const char en_text[] = N_("Do you want to repeat the last step and readjust the distance between the nozzle and heatbed?");
    string_view_utf8 translatedText = _(en_text);
    if (MsgBox(translatedText, Responses_YesNo, 1) == Response::No) {
        marlin_gcode("M104 S0"); // nozzle target
        marlin_gcode("M140 S0"); // bed target

        return WizardState_t::FIRSTLAY_RESULT;
    } else {
        static const char en_text[] = N_("Clean steel sheet.");
        string_view_utf8 translatedText = _(en_text);
        MsgBox(translatedText, Responses_Next);

        return WizardState_t::FIRSTLAY_MSBX_USEVAL;
    }
}

WizardState_t StateFnc_FIRSTLAY_RESULT() {
    //save eeprom flag
    eeprom_set_var(EEVAR_RUN_FIRSTLAY, variant8_ui8(0)); // clear first layer flag
    return WizardState_t::FINISH;

    //use folowing code when firstlay fails
#if 0
    //save eeprom flag
    static const char en_text[] = N_("The first layer calibration failed to finish. Double-check the printer's wiring, nozzle and axes, then restart the calibration.");
    string_view_utf8 translatedText = _(en_text);
    MsgBox(translatedText, Responses_Next);
    return WizardState_t::EXIT;
#endif //#if 0
}
