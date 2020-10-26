// screen_menu_errors.cpp

#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"

/*****************************************************************************/
//parent alias
using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN,
    MI_ES_12201, MI_ES_12202, MI_ES_12203, MI_ES_12204, MI_ES_12205, MI_ES_12206, MI_ES_12207, MI_ES_12208>;

class ScreenErrors : public Screen {
public:
    constexpr static const char *label = "Error Screens";
    ScreenErrors()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenErrors() {
    return ScreenFactory::Screen<ScreenErrors>();
}
