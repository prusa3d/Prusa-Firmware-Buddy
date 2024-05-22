/**
 * @file footer_items_heaters.hpp
 * @author Radek Vana
 * @brief heater items for footer
 * @date 2021-03-31
 */

#pragma once
#include "ifooter_item.hpp"
#include <array>
#include "footer_def.hpp"

class FooterItemHeater : public AddSuperWindow<FooterIconText_IntVal> {
    static void setDrawMode(footer::ItemDrawCnf cnf);

public:
    static void ResetDrawMode();
    static footer::ItemDrawType GetDrawType();
    static void SetDrawType(footer::ItemDrawType type);
    static bool IsZeroTargetDrawn();
    static void EnableDrawZeroTarget();
    static void DisableDrawZeroTarget();

    enum class HeatState : uint8_t {
        stable,
        heating,
        cooling,
        preheat,
        _last = preheat
    };

    // to be able to work with int
    struct StateAndTemps {
        bool no_tool; ///< True if no tool is picked
        HeatState state;
        uint16_t current : 12;
        uint16_t target_or_display : 12;

        constexpr StateAndTemps(int data = 0)
            : no_tool(data & 0b1)
            , state(HeatState((uint(data) >> 1) & 0b01111111))
            , current((uint(data) >> 8) & 0x0fff)
            , target_or_display((uint(data) >> 20) & 0x0fff) {}

        constexpr StateAndTemps(HeatState state, uint16_t current, uint16_t target_or_display, bool no_tool_)
            : no_tool(no_tool_)
            , state(state)
            , current(current)
            , target_or_display(target_or_display) {}

        constexpr int ToInt() const {
            uint32_t ret = target_or_display;
            ret = ret << 12;
            ret |= current;
            ret = ret << 7;
            ret |= (uint8_t(state) & 0b01111111);
            ret = ret << 1;
            ret |= no_tool ? 1 : 0;
            return ret;
        }
    };

protected:
    static constexpr int heating_difference = 2;

    static constexpr std::array<color_t, size_t(HeatState::_last) + 1> colors = { { COLOR_WHITE, COLOR_ORANGE, COLOR_AZURE, COLOR_GREEN } };

    /**
     * @brief Get the State object
     *
     * @param current    - current temperature
     * @param target     - target temperature (real one, printer regulates to this temperature)
     * @param display    - target to be displayed (we can preheat to 170 because of MBL, but show 215 as PLA temperature)
     * @param cold       - what temperature is considered as cold - blue flickering
     * @return HeatState - used for text colorization
     *          - stable  - white color
     *          - heating - orange color
     *          - cooling - blue flickering
     *          - preheat - green flickering
     */
    static HeatState getState(int current, int target, int display, int cold); // need signed values for comparison

    /// Buffer the exact size to fit temperature/target and degree Celsius
    using buffer_t = std::array<char, sizeof("333/333\xC2\xB0\x43")>;

    static string_view_utf8 static_makeViewIntoBuff(int value, buffer_t &buff);

public:
    virtual resized_t updateState() override;

    static inline color_t ColorFromState(HeatState st) { return colors[std::min(size_t(st), size_t(HeatState::_last))]; }

    FooterItemHeater(window_t *parent, const img::Resource *icon, view_maker_cb view_maker, reader_cb value_reader);
};
