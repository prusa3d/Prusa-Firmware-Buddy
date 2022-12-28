/**
 * @file screen_menu_eeprom_test.hpp
 * @brief screen to test eeprom
 */

#pragma once

#include "screen_menu.hpp"
#include "menu_items_open.hpp"
#include "WindowMenuSwitch.hpp"
#include "WindowMenuSpin.hpp"

enum class ClickCommandTest : intptr_t { Store,
    ValueChanged };

/**
 * @brief store current record
 */
class MI_STORE_EEPROM_TEST : public WI_LABEL_t {
    static constexpr const char *const label = N_("Store");

public:
    MI_STORE_EEPROM_TEST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

//TODO make macro to generate this, use same principe as in menu_opener.hpp
class SpinIntPart7 : public WiSpinInt {
    constexpr static const char *const label = "0xXX-- ---- ---- ----";

public:
    SpinIntPart7();
};

class SpinIntPart6 : public WiSpinInt {
    constexpr static const char *const label = "0x--XX ---- ---- ----";

public:
    SpinIntPart6();
};

class SpinIntPart5 : public WiSpinInt {
    constexpr static const char *const label = "0x---- XX-- ---- ----";

public:
    SpinIntPart5();
};

class SpinIntPart4 : public WiSpinInt {
    constexpr static const char *const label = "0x---- --XX ---- ----";

public:
    SpinIntPart4();
};

class SpinIntPart3 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- XX-- ----";

public:
    SpinIntPart3();
};

class SpinIntPart2 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- --XX ----";

public:
    SpinIntPart2();
};

class SpinIntPart1 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- ---- XX--";

public:
    SpinIntPart1();
};

class SpinIntPart0 : public WiSpinInt {
    constexpr static const char *const label = "0x---- ---- ---- --XX";

public:
    SpinIntPart0();
};

class EepromItems : public IWiSwitch {
    typename TextMemSpace_t::type ArrayMemSpace[EEVAR_CRC32]; //EEVAR_CRC32 == count
public:
    variant8_t var;

    EepromItems();

    IWiSwitch::Items_t GenerateItems();

protected:
    //show only integers .. TODO float, string
    virtual invalidate_t change(int /*dif*/) override;
};

/*****************************************************************************/
//ScreenMenuEepromTest parent alias
using ScreenMenuEepromTest__ = ScreenMenu<EFooter::Off, MI_RETURN, EepromItems, MI_STORE_EEPROM_TEST, SpinIntPart7, SpinIntPart6, SpinIntPart5, SpinIntPart4, SpinIntPart3, SpinIntPart2, SpinIntPart1, SpinIntPart0>;

class ScreenMenuEepromTest : public ScreenMenuEepromTest__ {
    constexpr static const char *label = "Eeprom Test";

    void storeValue();
    void updateValue();

public:
    ScreenMenuEepromTest();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override;
};
