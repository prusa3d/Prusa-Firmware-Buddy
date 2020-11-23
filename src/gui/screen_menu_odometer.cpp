/*
 * 	screen_version_info.cpp
 *
 *  Created on: 2019-10-14
 *      Author: Michal Rudolf
 */
//todo THIS SHOULD NOT BE MENU!!!
#include <stdlib.h>

#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"

static const constexpr HelperConfig HelpCfg = { 10, IDR_FNT_NORMAL };
enum : int {
    TEXT_MAX_LENGTH = 150
};

using Screen = ScreenMenu<EHeader::On, EFooter::On, HelpCfg, MI_RETURN>;

class ScreenMenuOdometer : public Screen {
    char text[TEXT_MAX_LENGTH];

public:
    constexpr static const char *label = N_("ODOMETER");
    ScreenMenuOdometer();
};

ScreenMenuOdometer::ScreenMenuOdometer()
    : Screen(_(label)) {

    header.SetIcon(IDR_PNG_info_16px);
    odometer.force_to_eeprom();
    float xyze[AXES];
    xyze = odometer.get_from_eeprom();
    snprintf(text, TEXT_MAX_LENGTH, "X        %.1f m\n\nY        %.1f m\n\nZ        %.1f m\n\nFilament %.1f m", xyze[0], xyze[1], xyze[2], xyze[3]);

    // this MakeRAM is safe - version_info_str is allocated in RAM for the lifetime of this
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)text));
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
