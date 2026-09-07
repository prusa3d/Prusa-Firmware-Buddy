/// @file
#include <gui/screen_menu_phase_stepping.hpp>

#include <img_resources.hpp>
#include <lang/i18n.h>

MI_PHASE_STEPPING_CALIBRATION::MI_PHASE_STEPPING_CALIBRATION()
    : MenuItemGcodeAction {
        _("Calibration"),
        "M1977",
        &img::calibrate_white_16x16,
    } {}

MI_PHASE_STEPPING_RESTORE_DEFAULTS::MI_PHASE_STEPPING_RESTORE_DEFAULTS()
    : MenuItemGcodeAction {
        _("Restore Defaults"),
        "M1977 D",
    } {}

ScreenMenuPhaseStepping::ScreenMenuPhaseStepping()
    : ScreenMenuPhaseSteppingBase {
        _("PHASE STEPPING"),
        &img::settings_16x16,
    } {}
