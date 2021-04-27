/**
 * @file footer_items_heaters.hpp
 * @author Radek Vana
 * @brief heater items for footer
 * @date 2021-03-31
 */

#pragma once
#include "ifooter_item.hpp"
#include <array>

class FooterItemHeater : public AddSuperWindow<FooterIconText_IntVal> {
    static footer::ItemDrawType draw_type;

public:
    static footer::ItemDrawType GetDrawType();
    static void SetDrawType(footer::ItemDrawType type);

    enum class HeatState : uint8_t {
        stable,
        heating,
        cooling,
        preheat,
        _last = preheat
    };

    //to be able to work with int
    struct StateAndTemps {
        HeatState state;
        uint16_t current : 12;
        uint16_t target_or_display : 12;

        constexpr StateAndTemps(int data = 0)
            : state(HeatState(uint(data) & 0xff))
            , current((uint(data) >> 8) & 0x0fff)
            , target_or_display((uint(data) >> 20) & 0x0fff) {}

        constexpr StateAndTemps(HeatState state, uint16_t current, uint16_t target_or_display)
            : state(state)
            , current(current)
            , target_or_display(target_or_display) {}

        constexpr int ToInt() const {
            uint32_t ret = target_or_display;
            ret = ret << 12;
            ret |= current;
            ret = ret << 8;
            ret |= uint8_t(state);
            return ret;
        }
    };

protected:
    static constexpr int heating_difference = 2;

    static constexpr std::array<color_t, size_t(HeatState::_last) + 1> colors = { { COLOR_WHITE, COLOR_ORANGE, COLOR_BLUE, COLOR_GREEN } };
    static HeatState getState(int current, int target, int display, int cold, int preheat); //need signed values for comparations
    static string_view_utf8 static_makeViewIntoBuff(int value, std::array<char, 10> &buff);

public:
    virtual resized_t updateState() override;

    static inline color_t ColorFromState(HeatState st) { return colors[std::min(size_t(st), size_t(HeatState::_last))]; }

    FooterItemHeater(window_t *parent, uint16_t icon_id, view_maker_cb view_maker, reader_cb value_reader);
};
