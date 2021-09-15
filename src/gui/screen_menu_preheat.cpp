// screen_menu_preheat.cpp

#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "filament.h"
#include "marlin_client.h"
#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

using Screen = ScreenMenu<EFooter::On, MI_RETURN,
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_PVB>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_NONE>>;

class ScreenMenuPreheat : public Screen {
public:
    constexpr static const char *label = N_("PREHEAT");
    ScreenMenuPreheat()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuPreheat() {
    return ScreenFactory::Screen<ScreenMenuPreheat>();
}

using ScreenNoRet = ScreenMenu<EFooter::On, HelpLines_None,
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_PVB>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_NONE>>;

class ScreenMenuPreheatNoRet : public ScreenNoRet {
public:
    constexpr static const char *label = N_("PREHEAT");
    ScreenMenuPreheatNoRet()
        : ScreenNoRet(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuPreheatNoRet() {
    return ScreenFactory::Screen<ScreenMenuPreheatNoRet>();
}
