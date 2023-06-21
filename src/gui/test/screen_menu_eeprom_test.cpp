/**
 * @file screen_menu_eeprom_test.cpp
 */

#include "screen_menu_eeprom_test.hpp"
#include "menu_spin_config.hpp"
#include "ScreenHandler.hpp"

MI_STORE_EEPROM_TEST::MI_STORE_EEPROM_TEST()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_STORE_EEPROM_TEST::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommandTest::Store);
}

SpinIntPart7::SpinIntPart7()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart6::SpinIntPart6()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart5::SpinIntPart5()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart4::SpinIntPart4()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart3::SpinIntPart3()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart2::SpinIntPart2()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart1::SpinIntPart1()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

SpinIntPart0::SpinIntPart0()
    : WiSpinInt(0, SpinCnf::two_digits_uint, _(label)) {}

EepromItems::EepromItems()
    : IWiSwitch(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)"RECORD"), nullptr, is_enabled_t::yes, is_hidden_t::no, GenerateItems())
    , var(0) // first records is int, no need to test it // TODO(ConfigStore): Not Migrated
{
}

IWiSwitch::Items_t EepromItems::GenerateItems() {
    const size_t SZ = EEVAR_CRC32; // TODO(ConfigStore): Not Migrated
    string_view_utf8 *pArr = (string_view_utf8 *)&ArrayMemSpace;
    for (size_t i = 0; i < SZ; ++i) {
        // pArr[i] = string_view_utf8::MakeCPUFLASH((const uint8_t *)eeprom_get_var_name(eevar_id(i))); // TODO(ConfigStore): Not Migrated
    }
    Items_t ret = Items_t(pArr, SZ);
    return ret;
}

invalidate_t EepromItems::change(int /*dif*/) {
    ++index;
    // index %= EEVAR_CRC32; // TODO(ConfigStore): Not Migrated
    bool can_show = false;
    while (!can_show) {
        var = 0; // TODO(ConfigStore): Not Migrated
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
            // index %= EEVAR_CRC32; // TODO(ConfigStore): Not Migrated
        }
    }

    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)ClickCommandTest::ValueChanged);

    return invalidate_t::yes;
}

void ScreenMenuEepromTest::storeValue() {
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
    // eeprom_set_var(eevar_id(Item<EepromItems>().GetIndex()), var);       // store to eeprom // TODO: Make work in config store
}

void ScreenMenuEepromTest::updateValue() {
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

ScreenMenuEepromTest::ScreenMenuEepromTest()
    : ScreenMenuEepromTest__(string_view_utf8::MakeCPUFLASH((const uint8_t *)label)) {
    updateValue();
}

void ScreenMenuEepromTest::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) {
    if (ev != GUI_event_t::CHILD_CLICK) {
        SuperWindowEvent(sender, ev, param);
        return;
    }

    switch (ClickCommandTest(intptr_t(param))) {
    case ClickCommandTest::Store:
        storeValue();
        break;
    case ClickCommandTest::ValueChanged:
        updateValue();
        break;
    }
}
