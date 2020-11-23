// screen_menu_info.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN,
#ifdef _DEBUG
    MI_STATISTIC_disabled, MI_FAIL_STAT_disabled, MI_SUPPORT_disabled,
#endif //_DEBUG
    MI_SYS_INFO, MI_SENSOR_INFO, MI_VERSION_INFO, MI_ODOMETER>;

//cannot move it to header - 'ScreenMenuInfo' has a field 'ScreenMenuInfo::<anonymous>' whose type uses the anonymous namespace [-Wsubobject-linkage]

class ScreenMenuInfo : public Screen {
public:
    constexpr static const char *label = N_("INFO");
    ScreenMenuInfo()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuInfo() {
    return ScreenFactory::Screen<ScreenMenuInfo>();
}
