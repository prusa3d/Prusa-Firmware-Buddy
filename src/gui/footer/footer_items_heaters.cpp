/**
 * @file footer_items_heaters.cpp
 * @author Radek Vana
 * @date 2021-03-31
 */

#include "footer_items_heaters.hpp"
#include "GuiDefaults.hpp"
#include "display_helper.h" // font_meas_text
#include <cmath>

bool FooterItemHeater::left_aligned = false;

bool FooterItemHeater::IsLeftAligned() { return left_aligned; }
void FooterItemHeater::LeftAlign() { left_aligned = true; }
void FooterItemHeater::ConstPositions() { left_aligned = false; }

FooterItemHeater::FooterItemHeater(window_t *parent, uint16_t icon_id, view_maker_cb view_maker, reader_cb value_reader)
    : AddSuperWindow<FooterIconText_IntVal>(parent, icon_id, view_maker, value_reader) {
}

//Must not contain buffer!!! every child must provide own buffer
string_view_utf8 FooterItemHeater::static_makeViewIntoBuff(int value, std::array<char, 10> &buff) {
    static constexpr const char *default_str = "  0/  0\177C";
    const StateAndTemps temps(value);
    const uint current = std::clamp(int(temps.current), 0, 999);
    const uint target_or_display = std::clamp(int(temps.target_or_display), 0, 999);

    size_t printed_chars = 0;

    if (left_aligned) {
        printed_chars = snprintf(buff.data(), buff.size(), "%u/%u\177C", current, target_or_display);
    } else {
        strncpy(buff.data(), default_str, buff.size());
    }

    if (printed_chars) {
        //left_aligned print successfull, add spaces to ensure fixed size
        *(buff.end() - 1) = '\0';
        for (; printed_chars < buff.size() - 1; ++printed_chars) {
            buff[printed_chars] = ' ';
        }
    } else {
        writeNums(current, target_or_display, buff.data()); //const size format
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff.data());
}

FooterItemHeater::HeatState FooterItemHeater::getState(int current, int target, int display, int cold, int preheat) {
    HeatState state = HeatState::stable;

    int temp_diff = target - current;
    if (temp_diff > heating_difference) {
        state = HeatState::heating;
    } else if (temp_diff < -heating_difference) {
        state = current > cold ? HeatState::cooling : HeatState::stable;
    }

    // preheat mode
    if ((display > target) && (std::abs(preheat - current) <= heating_difference)) {
        state = HeatState::preheat;
    }

    return state;
}

IFooterItem::resized_t FooterItemHeater::updateState() {
    const StateAndTemps temps(value);
    text.SetBlinkColor(ColorFromState(temps.state));
    return super::updateState();
}

// much faster than snprintf
void FooterItemHeater::writeNum(uint num, size_t index, char *buff) {
    for (int i = index + 2; i >= int(index); --i) {
        buff[i] = num % 10;
        buff[i] += '0';
        num /= 10;
    }

    //replace front zeroes with spaces
    for (int i = index; i <= int(index) + 1; ++i) {
        if (buff[i] == '0')
            buff[i] = ' ';
        else
            return;
    }
}

void FooterItemHeater::writeNums(uint actual, uint target, char *buff) {
    writeNum(actual, 0, buff);
    writeNum(target, 4, buff);
}
