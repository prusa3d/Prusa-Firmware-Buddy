/**
 * @file footer_items_heaters.cpp
 * @author Radek Vana
 * @date 2021-03-31
 */

#include "footer_items_heaters.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include <cmath>
#include "ScreenHandler.hpp"
#include "footer_eeprom.hpp"

footer::ItemDrawType FooterItemHeater::GetDrawType() {
    return footer::eeprom::get_item_draw_type();
}

void FooterItemHeater::SetDrawType(footer::ItemDrawType type) {
    footer::ItemDrawCnf cnf(footer::eeprom::load_item_draw_cnf());
    cnf.type = type;
    setDrawMode(cnf);
}

bool FooterItemHeater::IsZeroTargetDrawn() {
    return (footer::eeprom::get_item_draw_zero() == footer::draw_zero_t::yes) ? true : false;
}

void FooterItemHeater::EnableDrawZeroTarget() {
    footer::ItemDrawCnf cnf(footer::eeprom::load_item_draw_cnf());
    cnf.zero = footer::draw_zero_t::yes;
    setDrawMode(cnf);
}

void FooterItemHeater::DisableDrawZeroTarget() {
    footer::ItemDrawCnf cnf(footer::eeprom::load_item_draw_cnf());
    cnf.zero = footer::draw_zero_t::no;
    setDrawMode(cnf);
}

void FooterItemHeater::setDrawMode(footer::ItemDrawCnf cnf) {
    if (footer::eeprom::set(cnf) == changed_t::yes) {
        // item type is ItemNozzle or ItemBed
        // sadly this is static method, so i can do this in children (FooterItemHeater should not need to know about ItemNozzle/ItemBed)
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(footer::Item::nozzle));
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(footer::Item::bed));
    }
}

void FooterItemHeater::ResetDrawMode() {
    setDrawMode(footer::ItemDrawCnf::get_default());
}

FooterItemHeater::FooterItemHeater(window_t *parent, const img::Resource *icon, view_maker_cb view_maker, reader_cb value_reader)
    : AddSuperWindow<FooterIconText_IntVal>(parent, icon, view_maker, value_reader) {
}

// Must not contain buffer!!! every child must provide own buffer
string_view_utf8 FooterItemHeater::static_makeViewIntoBuff(int value, buffer_t &buff) {
    static constexpr const char *left_aligned_str = "%u/%u\xC2\xB0\x43";
    static constexpr const char *const_size_str = "%3u/%3u\xC2\xB0\x43";
    static constexpr const char *left_aligned_str_no_0 = "%u\xC2\xB0\x43";
    static constexpr const char *const_size_str_no_0 = "%3u\xC2\xB0\x43";

    const StateAndTemps temps(value);
    const uint current = std::clamp(int(temps.current), 0, 999);
    const uint target_or_display = std::clamp(int(temps.target_or_display), 0, 999);
    const bool no_tool = temps.no_tool;

    int printed_chars;

    if (no_tool) {
        printed_chars = snprintf(buff.data(), buff.size(), no_tool_str);
    } else if ((target_or_display == 0) && (!IsZeroTargetDrawn())) {
        const char *const str = (GetDrawType() == footer::ItemDrawType::static_) ? const_size_str_no_0 : left_aligned_str_no_0;
        printed_chars = snprintf(buff.data(), buff.size(), str, current);
    } else {
        const char *const str = (GetDrawType() == footer::ItemDrawType::static_) ? const_size_str : left_aligned_str;
        printed_chars = snprintf(buff.data(), buff.size(), str, current, target_or_display);
    }

    if (printed_chars <= 0) {
        buff[0] = '\0';
    } else if (GetDrawType() == footer::ItemDrawType::static_left_aligned) {
        // left_aligned print need to end with spaces ensure fixed size
        *(buff.end() - 1) = '\0';
        for (; size_t(printed_chars) < buff.size() - 1; ++printed_chars) {
            buff[printed_chars] = ' ';
        }
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

FooterItemHeater::HeatState FooterItemHeater::getState(int current, int target, int display, int cold) {
    HeatState state = HeatState::stable;

    int temp_diff = target - current;
    if (temp_diff > heating_difference) {
        state = HeatState::heating;
    } else if (temp_diff < -heating_difference) {
        state = current > cold ? HeatState::cooling : HeatState::stable;
    }

    // preheat mode
    if (display > (target + heating_difference)) {
        if ((std::abs(temp_diff) <= heating_difference)) {
            state = HeatState::preheat;
        }
    }

    return state;
}

resized_t FooterItemHeater::updateState() {
    const StateAndTemps temps(value);
    text.SetBlinkColor(ColorFromState(temps.state));
    return super::updateState();
}
