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

int first_decimal(float f) {
    return (int)(x * 10) % 10;
}

ScreenMenuOdometer::ScreenMenuOdometer()
    : Screen(_(label)) {

    header.SetIcon(IDR_PNG_info_16px);
    odometer.force_to_eeprom();
    const float x = odometer.get(0) * .001f;
    const float y = odometer.get(1) * .001f;
    const float z = odometer.get(2) * .001f;
    const float e = odometer.get(3) * .001f;

    /// FIXME this is not aligned if "Filament" is translated
    int written = snprintf(text, TEXT_MAX_LENGTH, "X        %d.%.1d m\n\nY        %d.%.1d m\n\nZ        %d.%.1d m\n\n", (int)x, first_decimal(x), (int)y, first_decimal(y), (int)z, first_decimal(z));
    if (written < 0)
        return;
    int written2 = snprintf(text + written, TEXT_MAX_LENGTH - written, "%s", _(filament_text));
    if (written2 < 0)
        return;
    written += written2;
    snprintf(text + written, TEXT_MAX_LENGTH - written, " %d.%.1d m", (int)e, first_decimal(e));

    // this MakeRAM is safe
    help.SetText(string_view_utf8::MakeRAM((const uint8_t *)text));
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
