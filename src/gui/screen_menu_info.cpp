// screen_menu_info.cpp

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "MItem_eeprom.hpp"

using Screen = ScreenMenu<EFooter::On, MI_RETURN,
    MI_I2C_TRANSMIT_RESULTS_HAL_OK,
    MI_I2C_TRANSMIT_RESULTS_HAL_ERROR,
    MI_I2C_TRANSMIT_RESULTS_HAL_BUSY,
    MI_I2C_TRANSMIT_RESULTS_HAL_TIMEOUT,
    MI_I2C_TRANSMIT_RESULTS_UNDEF,

    MI_I2C_RECEIVE_RESULTS_HAL_OK,
    MI_I2C_RECEIVE_RESULTS_HAL_ERROR,
    MI_I2C_RECEIVE_RESULTS_HAL_BUSY,
    MI_I2C_RECEIVE_RESULTS_HAL_TIMEOUT,
    MI_I2C_RECEIVE_RESULTS_UNDEF,
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
