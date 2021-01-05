// screen_menu .cpp

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
    static const constexpr char *label = N_("ODOMETER");
    static const constexpr char *filament_text = N_("Filament");
    ScreenMenuOdometer();
};

ScreenMenuOdometer::ScreenMenuOdometer()
    : Screen(_(label)) {

    header.SetIcon(IDR_PNG_info_16px);
    odometer.force_to_eeprom();
    int written = snprintf(text, TEXT_MAX_LENGTH, "X        %.1f m\n\nY        %.1f m\n\nZ        %.1f m\n\n", odometer.get(0) * .001f, odometer.get(1) * .001f, odometer.get(2) * .001f);
    if (written < 0)
        return;
    int written2 = snprintf(text + written, TEXT_MAX_LENGTH - written, "%s", _(filament_text));
    if (written2 < 0)
        return;
    written += written2;
    snprintf(text + written, TEXT_MAX_LENGTH - written, " %.1f m", odometer.get(3) * .001f);

    // this MakeRAM is safe
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)text));
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
