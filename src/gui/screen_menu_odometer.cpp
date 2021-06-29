// screen_menu_odometer.cpp

#include <stdlib.h>

#include "cmath_ext.h"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"

//static const constexpr HelperConfig HelpCfg = { 10, IDR_FNT_NORMAL };
enum : int {
    TEXT_MAX_LENGTH = 150
};

using Screen = ScreenMenu<EFooter::On, /* HelpCfg,*/ MI_RETURN>;

class ScreenMenuOdometer : public Screen {
    char text[TEXT_MAX_LENGTH];

public:
    static const constexpr char *label = N_("ODOMETER");
    ScreenMenuOdometer();
};

int first_decimal(float f) {
    return (int)(f * 10) % 10;
}

ScreenMenuOdometer::ScreenMenuOdometer()
    : Screen(_(label)) {

    header.SetIcon(IDR_PNG_info_16px);
    odometer_s.force_to_eeprom();
    const float x = odometer_s.get(0) * .001f;
    const float y = odometer_s.get(1) * .001f;
    const float z = odometer_s.get(2) * .001f;
    const float e = odometer_s.get(3) * .001f;

    static const constexpr char *filament_text = N_("Filament");
    string_view_utf8 filament_view = _(filament_text);
    const constexpr int transl_size = 30;
    char filament_text_translated[transl_size];
    filament_view.copyToRAM(filament_text_translated, transl_size);
    const int transl_length = filament_view.computeNumUtf8CharsAndRewind();

    const int padding = 1 + transl_length;
    char pad[30];
    int i = 0;
    for (; i < MIN(29, padding); ++i)
        pad[i] = ' ';
    pad[i] = 0;

    int written = snprintf(text, TEXT_MAX_LENGTH, "X%s%d.%.1d m\n\nY%s%d.%.1d m\n\nZ%s%d.%.1d m\n\n", pad, (int)x, first_decimal(x), pad, (int)y, first_decimal(y), pad, (int)z, first_decimal(z));
    if (written < 0)
        return;

    int written2 = snprintf(text + written, TEXT_MAX_LENGTH - written, "%s", filament_text_translated);
    if (written2 < 0)
        return;
    written += written2;
    snprintf(text + written, TEXT_MAX_LENGTH - written, " %d.%.1d m", (int)e, first_decimal(e));

    // this MakeRAM is safe
    //help.SetText(string_view_utf8::MakeRAM((const uint8_t *)text));
}

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenMenuOdometer>();
}
