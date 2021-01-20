// screen_menu_preheat.cpp

#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "filament.h"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN,
    MI_Filament<filament_t::PLA>,
    MI_Filament<filament_t::PETG>,
    MI_Filament<filament_t::ASA>,
    MI_Filament<filament_t::ABS>,
    MI_Filament<filament_t::PC>,
    MI_Filament<filament_t::FLEX>,
    MI_Filament<filament_t::HIPS>,
    MI_Filament<filament_t::PP>,
    MI_Filament<filament_t::NONE>>;

class ScreenMenuPreheat : public Screen {
public:
    constexpr static const char *label = N_("PREHEAT");
    ScreenMenuPreheat()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuPreheat() {
    return ScreenFactory::Screen<ScreenMenuPreheat>();
}

using ScreenNoRet = ScreenMenu<EHeader::Off, EFooter::On,
    MI_Filament<filament_t::PLA>,
    MI_Filament<filament_t::PETG>,
    MI_Filament<filament_t::ASA>,
    MI_Filament<filament_t::ABS>,
    MI_Filament<filament_t::PC>,
    MI_Filament<filament_t::FLEX>,
    MI_Filament<filament_t::HIPS>,
    MI_Filament<filament_t::PP>,
    MI_Filament<filament_t::NONE>>;

class ScreenMenuPreheatNoRet : public ScreenNoRet {
public:
    constexpr static const char *label = N_("PREHEAT");
    ScreenMenuPreheatNoRet()
        : ScreenNoRet(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuPreheatNoRet() {
    return ScreenFactory::Screen<ScreenMenuPreheatNoRet>();
}
