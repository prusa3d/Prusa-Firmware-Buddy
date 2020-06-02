// screen_menu_calibration.cpp

#include "gui.h"
#include "screen_menu.hpp"
#include "marlin_client.h"
#include "screens.h"

#include "menu_vars.h"
#include "eeprom.h"

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "marlin_vars.h"

class MI_Z_OFFSET : public WI_SPIN_t<float> {
    constexpr static const char *const label = "Z-offset";
    float get_Z_offset() {
        return marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET))->z_offset;
    }

public:
    MI_Z_OFFSET()
        : WI_SPIN_t<float>(get_Z_offset(), zoffset_fl_range, zoffset_fl_format, label, 0, true, false) {}
    virtual void OnClick() {
        eeprom_set_var(EEVAR_ZOFFSET, marlin_get_var(MARLIN_VAR_Z_OFFSET));
    }
    virtual bool Change(int dif) {
        bool ret = WI_SPIN_t<float>::Change(dif);
        marlin_set_z_offset(value);
        return ret;
    }
};

using Screen = screen_menu_data_t<false, true, false, MI_RETURN, MI_WIZARD, MI_Z_OFFSET, MI_AUTO_HOME, MI_MESH_BED,
    MI_SELFTEST, MI_CALIB_FIRST>;

static void init(screen_t *screen) {
    constexpr static const char *label = "Calibration";
    Screen::Create(screen, label);
}

screen_t screen_menu_calibration = {
    0,
    0,
    init,
    Screen::CDone,
    Screen::CDraw,
    Screen::CEvent,
    sizeof(Screen), //data_size
    0,              //pdata
};

extern "C" screen_t *const get_scr_menu_calibration() { return &screen_menu_calibration; }
