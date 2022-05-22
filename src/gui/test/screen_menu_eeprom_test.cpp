/**
 * @file screen_menu_eeprom_test.cpp
 * @author Radek Vana
 * @date 2022-01-11
 */

#include "screen_menu_eeprom_test.hpp"
#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "eeprom.h"
#include "WindowMenuSwitch.hpp"
#include "WindowMenuSpin.hpp"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"

enum class ClickCommand : intptr_t { Store,
    ValueChanged };

/**
 * @brief store current record
 *
 */
class MI_STORE : public WI_LABEL_t {
    static constexpr const char *const label = N_("Store");

public:
    MI_STORE()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}

protected:
    virtual void click(IWindowMenu &window_menu) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::Store);
    }
};

class SpinIntPart7 : public WiSpinInt {
    constexpr static const char *const label = "0xXX-- ---- ---- ----";

public:
    SpinIntPart7()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart6 : public WiSpinInt {
    constexpr static const char *const label = "0x--XX ---- ---- ----";

public:
    SpinIntPart6()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart5 : public WiSpinInt {
    constexpr static const char *const label = "0x---- XX-- ---- ----";

public:
    SpinIntPart5()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart4 : public WiSpinInt {
    constexpr static const char *const label = "0x---- --XX ---- ----";

public:
    SpinIntPart4()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart3 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- XX-- ----";

public:
    SpinIntPart3()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart2 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- --XX ----";

public:
    SpinIntPart2()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart1 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- ---- XX--";

public:
    SpinIntPart1()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class SpinIntPart0 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- ---- --XX";

public:
    SpinIntPart0()
        : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}
};

class EepromItems : public IWiSwitch {
    typename TextMemSpace_t::type ArrayMemSpace[EEVAR_CRC32]; //EEVAR_CRC32 == count
public:
    variant8_t var;

    EepromItems()
        : IWiSwitch(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)"RECORD"), 0, is_enabled_t::yes, is_hidden_t::no, GenerateItems())
        , var(eeprom_get_var(eevar_id(index))) // first records is int, no need to test it
    {
    }

    IWiSwitch::Items_t GenerateItems() {
        const size_t SZ = EEVAR_CRC32;
        string_view_utf8 *pArr = (string_view_utf8 *)&ArrayMemSpace;
        for (size_t i = 0; i < SZ; ++i) {
            pArr[i] = string_view_utf8::MakeCPUFLASH((const uint8_t *)eeprom_get_var_name(eevar_id(i)));
        }
        Items_t ret = Items_t(pArr, SZ);
        return ret;
    }

protected:
    //show only integers .. TODO float, string
    virtual invalidate_t change(int /*dif*/) override {
        ++index;
        index %= EEVAR_CRC32;
        bool can_show = false;
        while (!can_show) {
            var = eeprom_get_var(eevar_id(index));
            uint8_t type = variant8_get_type(var);
            switch (type) {
            case VARIANT8_I8:
            case VARIANT8_UI8:
            case VARIANT8_I16:
            case VARIANT8_UI16:
            case VARIANT8_I32:
            case VARIANT8_UI32:
                can_show = true;
                break;
            default:
                ++index;
                index %= EEVAR_CRC32;
            }
        }

        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommand::ValueChanged);

        return invalidate_t::yes;
    }
};

/*****************************************************************************/
//Screen
using Screen = ScreenMenu<EFooter::Off, MI_RETURN, EepromItems, MI_STORE, SpinIntPart7, SpinIntPart6, SpinIntPart5, SpinIntPart4, SpinIntPart3, SpinIntPart2, SpinIntPart1, SpinIntPart0>;

class ScreenMenuEepromTest : public Screen {
    constexpr static const char *label = "Eeprom Test";

    void storeValue() {
        uint32_t value = Item<SpinIntPart7>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart6>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart5>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart4>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart3>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart2>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart1>().GetVal();
        value <<= 4;
        value |= Item<SpinIntPart0>().GetVal();

        variant8_t var = variant8_ui32(value);                               // create variant as uint32_t
        variant8_set_type(&var, variant8_get_type(Item<EepromItems>().var)); // change variant type to correct one
        eeprom_set_var(eevar_id(Item<EepromItems>().GetIndex()), var);       // store to eeprom
    }

    void updateValue() {
        uint32_t value = variant8_get_ui32(Item<EepromItems>().var);
        Item<SpinIntPart7>().SetVal(int((value >> 28) & 0x0F));
        Item<SpinIntPart6>().SetVal(int((value >> 24) & 0x0F));
        Item<SpinIntPart5>().SetVal(int((value >> 20) & 0x0F));
        Item<SpinIntPart4>().SetVal(int((value >> 16) & 0x0F));
        Item<SpinIntPart3>().SetVal(int((value >> 12) & 0x0F));
        Item<SpinIntPart2>().SetVal(int((value >> 8) & 0x0F));
        Item<SpinIntPart1>().SetVal(int((value >> 4) & 0x0F));
        Item<SpinIntPart0>().SetVal(int((value >> 0) & 0x0F));
    }

public:
    ScreenMenuEepromTest()
        : Screen(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
        updateValue();
    }

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override {
        if (ev != GUI_event_t::CHILD_CLICK) {
            SuperWindowEvent(sender, ev, param);
            return;
        }

        switch (ClickCommand(intptr_t(param))) {
        case ClickCommand::Store:
            storeValue();
            break;
        case ClickCommand::ValueChanged:
            updateValue();
            break;
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuEepromTest() {
    return ScreenFactory::Screen<ScreenMenuEepromTest>();
}
