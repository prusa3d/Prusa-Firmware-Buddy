#include "screen_menu.hpp"
#include "filament.h"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "../lang/i18n.h"

template <FILAMENT_t T>
class MI_Filament : public WI_LABEL_t {
public:
    MI_Filament()
        : WI_LABEL_t(_(filaments[T].long_name), 0, true, false) {}

protected:
    virtual void click(Iwindow_menu_t & /*window_menu*/) override {
        const filament_t filament = filaments[T];
        marlin_gcode("M86 S1800"); // enable safety timer
        marlin_gcode_printf("M104 S%d", (int)filament.nozzle);
        marlin_gcode_printf("M140 S%d", (int)filament.heatbed);
        screen_close(); // skip this screen averytime
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN, MI_Filament<FILAMENT_PLA>, MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>, MI_Filament<FILAMENT_FLEX>, MI_Filament<FILAMENT_NONE>>;

static void init(screen_t *screen) {
    constexpr static const char *label = N_("PREHEAT");
    Screen::Create(screen, label);
}

screen_t screen_menu_preheat = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    nullptr,        //pdata
};

extern "C" screen_t *const get_scr_menu_preheat() { return &screen_menu_preheat; }
