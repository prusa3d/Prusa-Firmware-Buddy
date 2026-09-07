#include "footer_items_nozzle_bed.hpp"

#include "gui_time.hpp"
#include "filament.hpp"
#include <img_resources.hpp>
#include "footer_eeprom.hpp"
#include <cmath>
#include <option/has_indx.h>
#include <option/has_per_tool_temperatures.h>
#include <option/has_toolchanger.h>
#include <config_store/store_instance.hpp>
#include <utils/string_builder.hpp>
#include <common/nozzle_diameter.hpp>
#include <module/temperature/temp_defines.hpp>
#include <option/has_modular_bed.h>
#include <sensor_data.hpp>
#include <marlin_vars.hpp>
#include <display.hpp>

#if HAS_MODULAR_BED()
    #include <module/modular_heatbed.h>
#endif

FooterItemNozzle::FooterItemNozzle(window_t *parent)
    : FooterItemHeater(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
}

FooterItemNozzleDiameter::FooterItemNozzleDiameter(window_t *parent)
    : FooterIconText_FloatVal(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
}

FooterItemNozzlePWM::FooterItemNozzlePWM(window_t *parent)
    : FooterIconText_IntVal(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
}

FooterItemBed::FooterItemBed(window_t *parent)
    : FooterItemHeater(parent, &img::heatbed_16x16, static_makeView, static_readValue) {
#if HAS_MODULAR_BED()
    icon.Hide();
#endif
    updateValue();
}

void FooterItemBed::unconditionalDraw() {
    FooterItemHeater::unconditionalDraw();

#if HAS_MODULAR_BED()
    for (int x = 0; x < X_HBL_COUNT; x++) {
        for (int y = 0; y < Y_HBL_COUNT; y++) {
            uint16_t idx_mask = 1 << advanced_modular_bed->idx(x, y);
            bool enabled = last_enabled_bedlet_mask & idx_mask;
            bool warm = last_warm_bedlet_mask & idx_mask;

            if (enabled) {
                display::fill_rect(
                    Rect16(icon.Left() + x * 4, icon.Top() + icon.Height() - 4 - (y * 4), 3, 3),
                    COLOR_BRAND);
            } else if (warm) {
                uint px = icon.Left() + x * 4;
                uint py = icon.Top() + icon.Height() - 4 - (y * 4);

                display::set_pixel(point_ui16_t(px + 1, py), COLOR_BRAND);
                display::set_pixel(point_ui16_t(px + 1, py + 1), COLOR_BRAND);
                display::set_pixel(point_ui16_t(px + 1, py + 2), COLOR_BRAND);
                display::set_pixel(point_ui16_t(px, py + 1), COLOR_BRAND);
                display::set_pixel(point_ui16_t(px + 2, py + 1), COLOR_BRAND);
            } else {
                display::fill_rect(
                    Rect16(icon.Left() + x * 4, icon.Top() + icon.Height() - 4 - (y * 4), 3, 3),
                    COLOR_GRAY);
            }
        }
    }
#endif
}

changed_t FooterItemBed::updateValue() {
    changed_t ret = FooterItemHeater::updateValue();
#if HAS_MODULAR_BED()
    bool is_heating = marlin_vars().target_bed > 0;

    // if not heating, act as no heatbedlet is activated
    uint16_t enabled_bedlet_mask = is_heating ? marlin_vars().enabled_bedlet_mask : 0;
    if (enabled_bedlet_mask != last_enabled_bedlet_mask) {
        last_enabled_bedlet_mask = enabled_bedlet_mask;
        ret = changed_t::yes;
    }

    uint16_t warm_bedlet_mask = 0;
    for (int y = 0; y < Y_HBL_COUNT; ++y) {
        for (int x = 0; x < X_HBL_COUNT; ++x) {
            if (advanced_modular_bed->get_temp(x, y) > COLD) {
                warm_bedlet_mask |= 1 << advanced_modular_bed->idx(x, y);
            }
        }
    }

    if (last_warm_bedlet_mask != warm_bedlet_mask) {
        last_warm_bedlet_mask = warm_bedlet_mask;
        ret = changed_t::yes;
    }

#endif
    // It would seem that returning changed_t::yes is the proper way to
    // trigger an update. However, that doesn't work. Something to investigate.
    if (ret == changed_t::yes) {
        Invalidate();
    }
    return ret;
}

#if HAS_PER_TOOL_TEMPERATURES()

FooterItemAllNozzles::FooterItemAllNozzles(window_t *parent)
    : FooterIconText_IntVal(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
    icon.Hide();
}

uint FooterItemAllNozzles::nozzle_n = 0;

footer::ItemDrawType FooterItemAllNozzles::GetDrawType() {
    return footer::eeprom::get_item_draw_type();
}

int FooterItemAllNozzles::static_readValue() {
    /// Keep displayed value until switch_gui_time, so there is less flicker
    static uint keep_value = 0;

    ///< gui::GetTick() of last change of nozzle_n
    static uint32_t switch_gui_time = 0;

    // Wait for CYCLE_TIME
    uint32_t now = gui::GetTick();
    if ((now - switch_gui_time) > CYCLE_TIME) {
        switch_gui_time = now;

        // To prevent infinite loop if all tools are disabled
        const auto start = nozzle_n;

        // Switch to next enabled nozzle, will also change icon
        do {
            nozzle_n = (nozzle_n + 1) % PhysicalToolIndex::count;
        } while (!PhysicalToolIndex::from_raw(nozzle_n).is_enabled() && nozzle_n != start);

        // Update shown tool and temperature.
        // Guard against the uninitialized sentinel.
        const float temp = marlin_vars().hotend(PhysicalToolIndex::from_raw(nozzle_n)).temp_nozzle;
        const uint16_t temp_for_key = temp != TempInfo::celsius_uninitialized ? static_cast<uint16_t>(round(temp)) : 0;
        keep_value = (nozzle_n << 16) | temp_for_key;
    }

    return keep_value; // Return nozzle number in higher 16 bits and shown temperature in lower 16 bits
}

void FooterItemAllNozzles::unconditionalDraw() {
    FooterIconText_IntVal::unconditionalDraw();
    const uint16_t column_size = icon.Width() / PhysicalToolIndex::count; // 3 px per nozzle, 2 px column + 1 px space

    // White mark above currently shown tool
    display::fill_rect(
        Rect16(icon.Left() + nozzle_n * column_size, icon.Top(), column_size + 1, 2),
        COLOR_WHITE);

    // Individual nozzles
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        // Rectangle as high as temperature (can overwrite the white mark)
        const uint gray_column_max = (static_cast<uint>(COLD) * icon.Height() + (HEATER_XL_HOTEND_MAXTEMP / 2)) / HEATER_XL_HOTEND_MAXTEMP;
        // Guard against the uninitialized sentinel.
        const float temp = marlin_vars().hotend(tool).temp_nozzle;
        uint column_height = 0;
        if (temp != TempInfo::celsius_uninitialized) {
            column_height = (static_cast<uint>(round(temp)) * icon.Height() + (HEATER_XL_HOTEND_MAXTEMP / 2)) / HEATER_XL_HOTEND_MAXTEMP;
            column_height = std::clamp<uint>(column_height, 0, icon.Height());
        }
        uint gray_column_height = std::clamp<uint>(column_height, 0, gray_column_max);

        // Gray column down
        display::fill_rect(
            Rect16(icon.Left() + tool.to_raw() * column_size + 1, icon.Top() + icon.Height() - gray_column_height, column_size - 1, gray_column_height),
            COLOR_GRAY);
        // Orange column up
        if (column_height > gray_column_max) {
            display::fill_rect(
                Rect16(icon.Left() + tool.to_raw() * column_size + 1, icon.Top() + icon.Height() - column_height, column_size - 1, column_height - gray_column_max),
                COLOR_BRAND);
        }
    }
}

changed_t FooterItemAllNozzles::updateValue() {
    changed_t ret = FooterIconText_IntVal::updateValue();

    // It would seem that returning changed_t::yes is the proper way to
    // trigger an update. However, that doesn't work. Something to investigate.
    if (ret == changed_t::yes) {
        Invalidate();
    }
    return ret;
}

string_view_utf8 FooterItemAllNozzles::static_makeView(int value) {
    static constexpr const char *left_aligned_str = "T%u:%u\xC2\xB0\x43"; // degree Celsius
    static constexpr const char *const_size_str = "T%u:%3u\xC2\xB0\x43";

    static std::array<char, sizeof("T5:333\xC2\xB0\x43")> buff;

    const uint nozzle_n = value >> 16;
    const uint temperature = std::clamp(int(value & 0xffff), 0, 999);

    const char *const str = (GetDrawType() == footer::ItemDrawType::static_) ? const_size_str : left_aligned_str;
    int printed_chars = snprintf(buff.data(), buff.size(), str, nozzle_n + 1, temperature);

    if (printed_chars <= 0) {
        buff[0] = '\0';
    } else { // Dynamic is not allowed as it changes each second
        // left_aligned print need to end with spaces ensure fixed size
        buff.back() = '\0';
        for (; size_t(printed_chars) < buff.size() - 1; ++printed_chars) {
            buff[printed_chars] = ' ';
        }
    }
    return string_view_utf8::MakeRAM(buff.data());
}

#endif

int FooterItemNozzle::static_readValue() {
    static const uint cold = 45;

    const uint current = static_cast<uint>(round(marlin_vars().active_hotend().temp_nozzle));
    const uint target = static_cast<uint>(marlin_vars().active_hotend().target_nozzle);
    const uint display = static_cast<uint>(round(marlin_vars().active_hotend().display_nozzle));
#if HAS_TOOLCHANGER()
    const bool no_tool = std::holds_alternative<NoTool>(marlin_vars().active_extruder.get());
#else
    constexpr bool no_tool = false;
#endif

    HeatState state = getState(current, target, display, cold);
    StateAndTemps temps(state, current, display, no_tool);
    return temps.ToInt();
}

float FooterItemNozzleDiameter::static_readValue() {
    return match(
        marlin_vars().active_extruder.get(),
        [](VirtualToolIndex virtual_tool) -> float { return config_store().get_nozzle_diameter(virtual_tool.to_raw()); },
        [](NoTool) { return 0.0f; });
}

int FooterItemNozzlePWM::static_readValue() {
    return int(round(marlin_vars().active_hotend().pwm_nozzle.get() / 255.0f * 100.0f));
}

int FooterItemBed::static_readValue() {
    uint current = static_cast<uint>(round(marlin_vars().temp_bed));
    int16_t target = marlin_vars().target_bed;

    HeatState state = getState(current, target, target, COLD); // display == target will disable green blinking preheat
    StateAndTemps temps(state, current, target, false);
    return temps.ToInt();
}

// This methods cannot be one - need separate buffers
string_view_utf8 FooterItemNozzle::static_makeView(int value) {
    static buffer_t buff;
    return static_makeViewIntoBuff(value, buff);
}

string_view_utf8 FooterItemNozzleDiameter::static_makeView(float value) {
    static std::array<char, 8> buff;
    StringBuilder b(buff);
    if (std::holds_alternative<PhysicalToolIndex>(PhysicalToolIndex::currently_selected())) {
        b.append_float(value, { .max_decimal_places = nozzle_diameter_spin_config.max_decimal_places, .skip_zero_before_dot = false });
        b.append_string("mm");
    } else {
        b.append_string(no_tool_str);
    }
    return string_view_utf8::MakeRAM(buff.data());
}

string_view_utf8 FooterItemNozzlePWM::static_makeView(int value) {
    static constexpr const char *static_fmt = "%3" PRIi32 "%%";
    static constexpr const char *dynamic_fmt = "%" PRIi32 "%%";

    const auto draw_type = footer::eeprom::get_item_draw_type();

    static std::array<char, 5> buff;
    const char *fmt_str = (draw_type == footer::ItemDrawType::static_) ? static_fmt : dynamic_fmt;
    auto printed_chars = snprintf(buff.data(), buff.size(), fmt_str, value);

    // static_left_aligned print need to end with spaces ensure fixed size
    if (draw_type == footer::ItemDrawType::static_left_aligned) {
        for (; size_t(printed_chars) < (buff.size() - 1); ++printed_chars) {
            buff[printed_chars] = ' ';
        }
        buff[printed_chars] = '\0';
    }

    return string_view_utf8::MakeRAM((const unsigned char *)buff.data());
}

string_view_utf8 FooterItemBed::static_makeView(int value) {
    static buffer_t buff;
    return static_makeViewIntoBuff(value, buff);
}

#if HAS_INDX()
FooterItemNozzlePower::FooterItemNozzlePower(window_t *parent)
    : FooterIconText_IntVal(parent, &img::nozzle_16x16, static_makeView, static_readValue) {
}

int FooterItemNozzlePower::static_readValue() {
    return static_cast<int>(std::round(sensor_data().nozzle_power_W()));
}

string_view_utf8 FooterItemNozzlePower::static_makeView(int value) {
    static std::array<char, 7> buff;
    snprintf(buff.data(), buff.size(), "%dW", value);
    return string_view_utf8::MakeRAM(buff.data());
}
#endif
