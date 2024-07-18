#include "screen_printing_end_result.hpp"

#include <marlin_vars.hpp>
#include <gcode_info.hpp>
#include <format_print_will_end.hpp>
#include <img_resources.hpp>
#include "mmu2_toolchanger_common.hpp"
#include "print_time_module.hpp"

namespace {
constexpr const char *txt_print_started { N_("Print started") };
constexpr const char *txt_print_ended { N_("Print ended") };
constexpr const char *txt_consumed_material { N_("Consumed material") };
#if PRINTER_IS_PRUSA_XL
constexpr const char *txt_wipe_tower_pretranslated { N_("Prime tower %dg") };
#else
constexpr const char *txt_wipe_tower_pretranslated { N_("Wipe tower %dg") };
#endif

constexpr auto end_result_font { Font::small };

constexpr size_t column_left { 30 };
constexpr size_t column_right { GuiDefaults::ScreenWidth / 2 + column_left };
constexpr size_t column_width { 240 - 2 * column_left };

// constexpr size_t row_0 { 104 };
constexpr size_t row_height { 20 };

constexpr int16_t get_row(size_t row_0, size_t idx) {
    return row_0 + idx * row_height;
}

constexpr Rect16 get_printing_time_label_rect(int16_t row_0) { return { column_left, get_row(row_0, 0), column_width, row_height }; }
constexpr Rect16 get_printing_time_value_rect(int16_t row_0) { return { column_left, get_row(row_0, 1), column_width, row_height }; }

constexpr Rect16 get_print_started_label_rect(int16_t row_0) { return { column_left, get_row(row_0, 3), column_width, row_height }; }
constexpr Rect16 get_print_started_value_rect(int16_t row_0) { return { column_left, get_row(row_0, 4), column_width, row_height }; }

constexpr Rect16 get_print_ended_label_rect(int16_t row_0) { return { column_left, get_row(row_0, 6), column_width, row_height }; }
constexpr Rect16 get_print_ended_value_rect(int16_t row_0) { return { column_left, get_row(row_0, 7), column_width, row_height }; }

constexpr size_t consumed_material_row { 3 };

constexpr Rect16 get_consumed_material_label_rect(int16_t row_0) { return { column_right, get_row(row_0, consumed_material_row), column_width, row_height }; }

constexpr Rect16 get_consumed_material_rect(int16_t row_0, size_t idx) {
    return Rect16 { column_right, static_cast<int16_t>(get_row(row_0, consumed_material_row + 1 + idx)), column_width, row_height };
}

// row is going to be set dynamically
constexpr Rect16 get_consumed_wipe_tower_value_rect(int16_t row_0) { return { column_right, get_row(row_0, 7), column_width, row_height }; }

constexpr size_t middle_of_buttons { 185 + 40 };
constexpr auto arrow_right_res { &img::arrow_right_10x16 };
constexpr Rect16 arrow_right_rect { column_right + column_width, middle_of_buttons - arrow_right_res->h / 2, arrow_right_res->w, arrow_right_res->h };

void handle_timestamp_text_item(MarlinVariableLocked<time_t> &time_holder, EndResultBody::DateBufferT &buffer, window_text_t &text_value) {
    const auto time_format = time_tools::get_time_format();
    struct tm print_tm;
    time_holder.execute_with([&](const time_t &print_time) {
        localtime_r(&print_time, &print_tm);
    });

    print_tm.tm_min += time_tools::calculate_total_timezone_offset_minutes();

    const time_t adjusted_print_time = mktime(&print_tm);
    localtime_r(&adjusted_print_time, &print_tm);

    FormatMsgPrintWillEnd::Date(buffer.data(), buffer.size(), &print_tm, time_format == time_tools::TimeFormat::_24h, FormatMsgPrintWillEnd::ISO);

    text_value.SetText(_(buffer.data()));
    text_value.Show();
}

} // namespace

EndResultBody::EndResultBody(window_t *parent, Rect16 rect)
    : window_frame_t(parent, rect)
    , printing_time_label(this, get_printing_time_label_rect(get_row_0()), is_multiline::no, is_closed_on_click_t::no, _(txt_printing_time))
    , printing_time_value(this, get_printing_time_value_rect(get_row_0()), is_multiline::no)

    , print_started_label(this, get_print_started_label_rect(get_row_0()), is_multiline::no, is_closed_on_click_t::no, _(txt_print_started))
    , print_started_value(this, get_print_started_value_rect(get_row_0()), is_multiline::no)

    , print_ended_label(this, get_print_ended_label_rect(get_row_0()), is_multiline::no, is_closed_on_click_t::no, _(txt_print_ended))
    , print_ended_value(this, get_print_ended_value_rect(get_row_0()), is_multiline::no)

    , consumed_material_label(this, get_consumed_material_label_rect(get_row_0()), is_multiline::no, is_closed_on_click_t::no, _(txt_consumed_material))
    , consumed_material_values(
          [&]<size_t... Is>(std::index_sequence<Is...>) -> decltype(consumed_material_values) {
              //  just a fancy template way to init array of items without default constructor
              return { (window_text_t { this, get_consumed_material_rect(get_row_0(), Is), is_multiline::no })... };
          }(std::make_index_sequence<std::tuple_size_v<decltype(consumed_material_values)>>()))
    , consumed_wipe_tower_value(this, get_consumed_wipe_tower_value_rect(get_row_0()), is_multiline::no)
    , arrow_right(this, arrow_right_rect, arrow_right_res)
    , progress_txt(this, get_progress_txt_rect(get_row_0())) {

    printing_time_label.SetTextColor(COLOR_SILVER);
    print_started_label.SetTextColor(COLOR_SILVER);
    print_ended_label.SetTextColor(COLOR_SILVER);
    consumed_material_label.SetTextColor(COLOR_SILVER);

    printing_time_label.set_font(end_result_font);
    print_started_label.set_font(end_result_font);
    print_ended_label.set_font(end_result_font);
    consumed_material_label.set_font(end_result_font);
    printing_time_value.set_font(end_result_font);
    print_started_value.set_font(end_result_font);
    print_ended_value.set_font(end_result_font);
    for (auto &consumed_material_value : consumed_material_values) {
        consumed_material_value.set_font(end_result_font);
    }
    consumed_wipe_tower_value.set_font(end_result_font);

    progress_txt.SetAlignment(progress_alignment);
    progress_txt.set_font(progress_font);
}

Rect16 EndResultBody::get_progress_txt_rect(int16_t row_0) {
    static constexpr size_t progress_top_decrease { 4 };
    static_assert(progress_top_decrease <= extra_top_space); // otherwise try increasing extra top space
    return { 300, static_cast<int16_t>(get_row(row_0, 0) - progress_top_decrease), 135, 54 };
}

int16_t EndResultBody::get_row_0() const {
    return GetRect().Top() + extra_top_space;
}

void EndResultBody::Show() {

    auto &gcode { GCodeInfo::getInstance() };

    PrintTime::print_formatted_duration(marlin_vars().print_duration.get(), { printing_time_value_buffer }, true);

    printing_time_label.Show();
    printing_time_value.Show();
    printing_time_value.SetText(_(printing_time_value_buffer.data()));

    print_started_label.Show();
    print_ended_label.Show();

    handle_timestamp_text_item(marlin_vars().print_start_time, print_started_value_buffer, print_started_value);
    handle_timestamp_text_item(marlin_vars().print_end_time, print_ended_value_buffer, print_ended_value);

    handle_consumed_material_showing(gcode);

    arrow_right.Show();

    window_t::Show();
}

void EndResultBody::handle_consumed_material_showing(const GCodeInfo &gcode) {

    // holds how many extruders were printing with specified grammage that's big enough to show
    const size_t num_extruders_with_valid_grams = std::ranges::count_if(gcode.get_per_extruder_info(), [&](auto &elem) {
        return elem.filament_used_g.has_value() && std::lround(elem.filament_used_g.value()) >= minimum_grams_valid;
    });

    const bool has_valid_wipe_tower_grams {
        [&]() {
#if EXTRUDERS > 1
            return gcode.get_filament_wipe_tower_g().has_value() && std::lround(gcode.get_filament_wipe_tower_g().value()) >= minimum_grams_valid;
#else
            return false;
#endif
        }()
    };

    setup_consumed_material_fields(num_extruders_with_valid_grams, has_valid_wipe_tower_grams);

    consumed_material_label.Show();
    if (num_extruders_with_valid_grams == 0 && !has_valid_wipe_tower_grams) {
        auto &buff { consumed_material_values_buffers[0] };
        snprintf(buff.data(), buff.size(), "---");
        consumed_material_values[0].SetText(_(buff.data()));
        consumed_material_values[0].Show();
    } else {
        handle_consumed_tool_fields(gcode, num_extruders_with_valid_grams);
        handle_wipe_tower_showing(gcode, has_valid_wipe_tower_grams);
    }
}

void EndResultBody::handle_wipe_tower_showing([[maybe_unused]] const GCodeInfo &gcode, [[maybe_unused]] bool has_valid_wipe_tower_grams) {
#if EXTRUDERS > 1
    // wipe tower
    if (has_valid_wipe_tower_grams) {
        const auto used_g = static_cast<int>(std::lround(gcode.get_filament_wipe_tower_g().value()));
        const string_view_utf8 str = _(txt_wipe_tower_pretranslated).formatted(wipe_tower_params, used_g);
        consumed_wipe_tower_value.SetText(str);
        consumed_wipe_tower_value.Show();
    }
#endif
    // else keep it hidden
}

void EndResultBody::handle_consumed_tool_fields(const GCodeInfo &gcode, size_t num_extruders_with_valid_grams) {
    if (num_extruders_with_valid_grams > 0) {
        for (size_t i = 0, consumed_material_line_idx = 0; i < EXTRUDERS; ++i) {
            const auto &ext_info { gcode.get_extruder_info(i) };
            if (!ext_info.used() || !ext_info.filament_used_g.has_value() || std::lround(ext_info.filament_used_g.value()) < minimum_grams_valid) {
                continue;
            }

            const auto &fname { ext_info.filament_name };
            const auto used_g = static_cast<int>(std::lround(ext_info.filament_used_g.value())); // guaranteed to have value, see above guard

            auto print_fname = [&]() {
                return fname.has_value() ? fname.value().data() : "---";
            };

            auto &buff { consumed_material_values_buffers[consumed_material_line_idx] };

            const bool show_t_label {
#if EXTRUDERS > 1
                []() {
    #if HAS_MMU2()
                    if (MMU2::mmu2.Enabled()) {
                        return true;
                    }
    #endif
    #if HAS_TOOLCHANGER()
                    if (prusa_toolchanger.is_toolchanger_enabled()) {
                        return true;
                    }
    #endif
                    return false;
                }()

#else
                false
#endif
            };

            if (show_t_label) {
                snprintf(buff.data(), buff.size(), "T%d %s %dg", i + 1, print_fname(), used_g);
            } else {
                snprintf(buff.data(), buff.size(), "%s %dg", print_fname(), used_g);
            }

            consumed_material_values[consumed_material_line_idx].SetText(_(buff.data()));
            consumed_material_values[consumed_material_line_idx].Show();
            ++consumed_material_line_idx;
        }
    }
}

void EndResultBody::setup_consumed_material_fields(size_t num_extruders_with_valid_grams, bool has_valid_wipe_tower_grams) {
    static constexpr auto keep_progress_threshold { 2 }; // anymore used gcodes will trigger hiding progress text and moving right column to the top;

    // setup fields
    if (num_extruders_with_valid_grams > keep_progress_threshold) {
        // prepare right column to be without progress_txt
        progress_txt.Hide();

        auto place_one = [](window_text_t &text_field, Rect16 default_rect, Rect16::Top_t new_top) {
            default_rect.set(new_top);
            text_field.SetRect(default_rect);
        };

        place_one(consumed_material_label, get_consumed_material_label_rect(get_row_0()), get_row(get_row_0(), 0));

        for (size_t i = 0; i < num_extruders_with_valid_grams; ++i) {
            place_one(consumed_material_values[i], get_consumed_material_rect(get_row_0(), i), get_row(get_row_0(), i + 1));
        }

        if (has_valid_wipe_tower_grams) {
            place_one(consumed_wipe_tower_value, get_consumed_wipe_tower_value_rect(get_row_0()), get_row(get_row_0(), num_extruders_with_valid_grams + 2));
        }
    } else {
        // Right column contains progress_txt, so reset places

        progress_txt.Show();

        consumed_material_label.SetRect(get_consumed_material_label_rect(get_row_0()));
        for (size_t i = num_extruders_with_valid_grams; i < std::size(consumed_material_values); ++i) {
            consumed_material_values[i].SetRect(get_consumed_material_rect(get_row_0(), i));
        }

        // show wipe tower info
        if (has_valid_wipe_tower_grams) {

            Rect16 tmp = get_consumed_wipe_tower_value_rect(get_row_0());

            if (num_extruders_with_valid_grams > 0) {
                // print after individual tools with a blank space in between
                tmp.set(Rect16::Top_t { static_cast<int16_t>(get_row(get_row_0(), consumed_material_row + num_extruders_with_valid_grams + 2)) });

            } else {
                // don't do blank space if we're only showing wipe tower
                tmp.set(Rect16::Top_t { static_cast<int16_t>(get_row(get_row_0(), consumed_material_row + 1)) });
            }

            consumed_wipe_tower_value.SetRect(tmp);
        }
    }
}

void EndResultBody::Hide() {
    printing_time_label.Hide();
    printing_time_value.Hide();

    print_started_label.Hide();
    print_started_value.Hide();

    print_ended_label.Hide();
    print_ended_value.Hide();

    consumed_material_label.Hide();
    for (auto &consumed_material_value : consumed_material_values) {
        consumed_material_value.Hide();
    }

    consumed_wipe_tower_value.Hide();

    arrow_right.Hide();
    progress_txt.Hide();

    window_t::Hide();
}

void EndResultBody::windowEvent(window_t *, GUI_event_t event, void *param) {
    if (event == GUI_event_t::ENC_UP || event == GUI_event_t::CLICK || event == GUI_event_t::TOUCH_CLICK) {
        if (GetParent()) {
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CHANGED, param);
        }
    }
}
