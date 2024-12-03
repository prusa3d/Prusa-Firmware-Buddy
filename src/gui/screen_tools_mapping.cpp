#include "screen_tools_mapping.hpp"
#include <logging/log.hpp>
#include <marlin_client.hpp>
#include <string.h>
#include <window_msgbox.hpp>
#include <printers.h>
#include <RAII.hpp>
#include <ScreenHandler.hpp>
#include <screen_menu_filament_changeall.hpp>
#include <img_resources.hpp>
#include <marlin_print_preview.hpp>
#include <utility_extensions.hpp>
#include "mmu2_toolchanger_common.hpp"
#include <tools_mapping.hpp>
#include <print_utils.hpp>
#include <filament_sensors_handler.hpp>

namespace {

static_assert(window_line_connector::max_one_side_points >= ToolsMappingBody::max_item_rows, "Can't fit all rows into connector");

/**
 * @brief Main text zone is as follows

 *              col_0_text                           col_1_text
 * text_side_margin | text_col_width | middle_zone_width | text_col_width | text_side_margin
*                                     >  middle margin  <
 * similarly for progress bar, except it has different names for the parts (text -> line)
 */

constexpr size_t middle_zone_width { 80 };
constexpr size_t middle_connector_margin { 5 }; // from each side of middle zone for middle_connector
constexpr size_t connector_line_height { 2 };

constexpr size_t line_side_margin { 15 }; // progress bar line
constexpr size_t line_height { 3 };
constexpr size_t line_col_width { (GuiDefaults::ScreenWidth - middle_zone_width - 2 * line_side_margin) / 2 };

constexpr size_t text_side_margin { 18 };
constexpr size_t text_height { 21 }; // not tied to font because there would be +- constants to make it look nice anyway
constexpr size_t text_col_width { (GuiDefaults::ScreenWidth - middle_zone_width - 2 * text_side_margin) / 2 };
constexpr size_t color_margin { 2 }; // within text_col_width

constexpr size_t col_0_text = text_side_margin;
constexpr size_t col_1_text = col_0_text + text_col_width + middle_zone_width;
constexpr size_t col_0_line = line_side_margin;
constexpr size_t col_1_line = col_0_line + line_col_width + middle_zone_width;

constexpr size_t text_row_separation { 30 };
constexpr size_t row_headers = 45;
constexpr size_t row_lines = row_headers + 25;
constexpr size_t row_first_item = row_lines + 12;
constexpr size_t row_guide_text = row_first_item + ToolsMappingBody::max_item_rows * text_row_separation + text_height / 2; // center it

constexpr Rect16 left_header_rect { col_0_text, row_headers, text_col_width, text_height };
constexpr Rect16 right_header_rect { col_1_text, row_headers, text_col_width, text_height };
constexpr Rect16 left_line_rect { col_0_line, row_lines, line_col_width, line_height };
constexpr Rect16 right_line_rect { col_1_line, row_lines, line_col_width, line_height };

constexpr Rect16 middle_connectors_rect { col_0_text + text_col_width + middle_connector_margin, row_first_item,
    middle_zone_width - 2 * middle_connector_margin, text_height *ToolsMappingBody::max_item_rows };

constexpr Rect16 get_left_gcode_rect(size_t idx) {
    return { col_0_text, static_cast<int16_t>(row_first_item + idx * text_row_separation), text_col_width, text_height };
}

constexpr size_t color_height { text_height - 2 * color_margin };

constexpr Rect16 get_left_gcode_color_rect(size_t idx) {
    return { col_0_text + text_col_width - color_height - color_margin, static_cast<int16_t>(row_first_item + color_margin + idx * text_row_separation), color_height, color_height };
}

constexpr Rect16 get_right_phys_rect(size_t idx) {
    return { col_1_text, static_cast<int16_t>(row_first_item + idx * text_row_separation), text_col_width, text_height };
}

constexpr size_t alert_icon_size { 16 };

constexpr uint16_t get_icon_row(size_t idx) {
    return row_first_item + text_row_separation * idx
        + text_height / 2 // center point of row
        - alert_icon_size / 2; // offset up
}

constexpr Rect16 get_left_gcode_icon_rect(size_t idx) {
    return { col_0_text + text_col_width + middle_connector_margin, static_cast<int16_t>(get_icon_row(idx)), alert_icon_size, alert_icon_size };
}

constexpr Rect16 get_right_phys_icon_rect(size_t idx) {
    return { col_1_text - alert_icon_size - middle_connector_margin, static_cast<int16_t>(get_icon_row(idx)), alert_icon_size, alert_icon_size };
}

constexpr Rect16 bottom_guide_rect { text_side_margin, row_guide_text, GuiDefaults::ScreenWidth - 2 * text_side_margin, text_height };
constexpr Rect16 bottom_icon_rect { 0, row_guide_text + (text_height - alert_icon_size) / 2, alert_icon_size, alert_icon_size }; // this rect will have 'left' modified by the current strlen of text in bottom_guide (needs to be 0)

constexpr Rect16 bottom_radio_rect { GuiDefaults::GetButtonRect(GuiDefaults::DialogFrameRect) };

constexpr const img::Resource *unassigned_filament_icon { &img::nok_color_16x16 };
constexpr const img::Resource *mismatched_nozzles_icon { &img::error_16x16 };
constexpr const img::Resource *mismatched_filaments_icon { &img::warning_16x16 };
constexpr const img::Resource *unloaded_tools_icon { &img::question_16x16 };

void set_hovered(window_text_t &item, window_colored_rect *color) {
    item.SetTextColor(COLOR_BLACK);
    item.SetBackColor(COLOR_WHITE);
    if (color) {
        color->set_parent_color(COLOR_WHITE);
    }
}

void set_idle(window_text_t &item, window_colored_rect *color) {
    item.SetTextColor(COLOR_WHITE);
    item.SetBackColor(COLOR_BLACK);
    if (color) {
        color->set_parent_color(COLOR_BLACK);
    }
}

void set_selected(window_text_t &item, window_colored_rect *color) {
    item.SetTextColor(COLOR_WHITE);
    item.SetBackColor(COLOR_ORANGE);
    if (color) {
        color->set_parent_color(COLOR_BLACK);
    }
}

void set_radio_idx(RadioButton &radio, size_t idx) {
    radio.EnableDrawingSelected();
    radio.SetBtnIndex(idx);
    radio.Invalidate();
}

void disable_radio(RadioButton &radio) {
    radio.DisableDrawingSelected();
    radio.Invalidate();
}

float get_nozzle_diameter([[maybe_unused]] size_t idx) {
#if HAS_TOOLCHANGER()
    return static_cast<float>(config_store().get_nozzle_diameter(idx));
#elif HAS_MMU2()
    return static_cast<float>(config_store().get_nozzle_diameter(0));
#endif
}

void print_right_tool_into_buffer(size_t idx, std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, bool drawing_nozzles) {
    // IDX here means REAL

    const FilamentType loaded_filament_type = config_store().get_filament_type(idx);
    FilamentTypeParameters::Name loaded_filament_name = loaded_filament_type.parameters().name;

#if HAS_MMU2()
    static constexpr std::array unknown_filament_names = {
        "FIL1", "FIL2", "FIL3", "FIL4", "FIL5"
    };

    // Upon request from the Content team - if we get "---", translate it into FILAM - a crude and awful hack :(
    if (loaded_filament_type == FilamentType::none && idx < unknown_filament_names.size()) {
        strlcpy(loaded_filament_name.data(), unknown_filament_names[idx], filament_name_buffer_size);
    }
#endif

    snprintf(text_buffers[idx].data(), ToolsMappingBody::max_item_text_width, "%hhu. %-5.5s", static_cast<uint8_t>(idx + 1), loaded_filament_name.data());

    if (drawing_nozzles) {
        const auto cur_strlen = strlen(text_buffers[idx].data());
        snprintf(text_buffers[idx].data() + cur_strlen, ToolsMappingBody::max_item_text_width - cur_strlen, " %-4.2f", static_cast<double>(get_nozzle_diameter(idx)));
    }
}

//  IDX here means REAL
window_text_t make_right_phys_text(size_t idx, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, bool drawing_nozzles) {

    print_right_tool_into_buffer(idx, text_buffers, drawing_nozzles);
    window_text_t wtxt { parent, get_right_phys_rect(idx), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM(text_buffers[idx].data()) };
    if (!is_tool_enabled(idx)) {
        wtxt.Hide();
    }
    return wtxt;
}

template <size_t... Is>
std::array<window_text_t, sizeof...(Is)> make_right_phys_text(std::index_sequence<Is...>, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, bool drawing_nozzles) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_right_phys_text(Is, parent, text_buffers, drawing_nozzles))... };
}

window_text_t make_left_gcode_text(size_t idx, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, GCodeInfo &gcode, bool drawing_nozzles) {

    const auto fil_name = gcode.get_extruder_info(idx).filament_name;
    snprintf(text_buffers[idx].data(), ToolsMappingBody::max_item_text_width, "%hhu. %-5.5s", static_cast<uint8_t>(idx + 1), fil_name.has_value() ? fil_name->data() : "---");

    if (drawing_nozzles) {
        const auto cur_strlen = strlen(text_buffers[idx].data());
        if (gcode.get_extruder_info(idx).nozzle_diameter.has_value()) {
            snprintf(text_buffers[idx].data() + cur_strlen, ToolsMappingBody::max_item_text_width - cur_strlen, " %-4.2f", static_cast<double>(gcode.get_extruder_info(idx).nozzle_diameter.value()));
        } else {
            snprintf(text_buffers[idx].data() + cur_strlen, ToolsMappingBody::max_item_text_width - cur_strlen, " %-4.4s", "?.??");
        }
    }

    window_text_t wtxt { parent, get_left_gcode_rect(idx), is_multiline::no, is_closed_on_click_t::no, string_view_utf8::MakeRAM(text_buffers[idx].data()) };
    if (!gcode.get_extruder_info(idx).used()) {
        wtxt.Hide();
    }
    return wtxt;
}

template <size_t... Is>
std::array<window_text_t, sizeof...(Is)> make_left_gcode_text(std::index_sequence<Is...>, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, GCodeInfo &gcode, bool are_all_nozzles_same) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_left_gcode_text(Is, parent, text_buffers, gcode, are_all_nozzles_same))... };
}

window_colored_rect make_left_gcode_color(size_t idx, window_t *parent, GCodeInfo &gcode) {
    window_colored_rect colored { parent, get_left_gcode_color_rect(idx) };
    if (auto extruder_info = gcode.get_extruder_info(idx);
        extruder_info.used() && extruder_info.extruder_colour.has_value()) {
        colored.SetBackColor(*extruder_info.extruder_colour);
    } else {
        colored.Hide();
    }
    return colored;
}

template <size_t... Is>
std::array<window_colored_rect, sizeof...(Is)> make_left_gcode_color(std::index_sequence<Is...>, window_t *parent, GCodeInfo &gcode) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_left_gcode_color(Is, parent, gcode))... };
}

window_icon_t make_left_gcode_icon(size_t idx, window_t *parent) {
    window_icon_t icon { parent, get_left_gcode_icon_rect(idx), nullptr };
    return icon;
}

template <size_t... Is>
std::array<window_icon_t, sizeof...(Is)> make_left_gcode_icon(std::index_sequence<Is...>, window_t *parent) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_left_gcode_icon(Is, parent))... };
}

window_icon_t make_right_phys_icon(size_t idx, window_t *parent) {
    window_icon_t icon { parent, get_right_phys_icon_rect(idx), nullptr };
    return icon;
}

template <size_t... Is>
std::array<window_icon_t, sizeof...(Is)> make_right_phys_icon(std::index_sequence<Is...>, window_t *parent) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_right_phys_icon(Is, parent))... };
}

Response tools_mapping_box(bool &querying_user, const string_view_utf8 &msg, PhaseResponses responses, size_t default_button = 0) {
    AutoRestore ar(querying_user, true);
    const PhaseTexts labels = { get_response_text(responses[0]), get_response_text(responses[1]), get_response_text(responses[2]), get_response_text(responses[3]) };
    MsgBoxBase msgbox(GuiDefaults::DialogFrameRect, responses, default_button, &labels, msg, is_multiline::yes);
    msgbox.set_text_alignment(Align_t::Center());
    Screens::Access()->gui_loop_until_dialog_closed();
    return msgbox.GetResult();
}

bool all_nozzles_same(GCodeInfo &gcode_info) {
    // precondition: at least 1 used gcode
    float first_value { 0.0 };
    bool initialized { false };

    auto nozzles_are_matching = [](float lhs, float rhs) {
        float nozzle_diameter_distance = std::abs(lhs - rhs);
        return nozzle_diameter_distance <= 0.001f;
    };
    // check gcodes
    EXTRUDER_LOOP() {
        if (!gcode_info.get_extruder_info(e).used()) {
            continue;
        }

        if (!gcode_info.get_extruder_info(e).nozzle_diameter.has_value()) {
            // used but not given diameter -> can't guarantee all same
            return false;
        }

        if (!initialized) {
            first_value = gcode_info.get_extruder_info(e).nozzle_diameter.value();
            initialized = true;
            continue;
        }

        if (!nozzles_are_matching(first_value, gcode_info.get_extruder_info(e).nozzle_diameter.value())) {
            return false;
        }
    }

    // check physicals
    EXTRUDER_LOOP() {
        if (!is_tool_enabled(e)) {
            continue;
        }

        if (!nozzles_are_matching(first_value, get_nozzle_diameter(e))) {
            return false;
        }
    }
    return true;
}

} // namespace

ToolsMappingBody::ToolsMappingBody(window_t *parent, GCodeInfo &gcode_info)
    : window_t(parent, GuiDefaults::RectScreenNoHeader)
    , drawing_nozzles(!all_nozzles_same(gcode_info))
    , left_header(parent, left_header_rect, is_multiline::no, is_closed_on_click_t::no, _("G-Code filaments"))
    , right_header(parent, right_header_rect, is_multiline::no, is_closed_on_click_t::no,
#if not HAS_MMU2()
          _("Printer tools")
#else
          _("MMU filament")
#endif
              )
    , left_line(parent, left_line_rect, COLOR_ORANGE, COLOR_GRAY)
    , right_line(parent, right_line_rect, COLOR_ORANGE, COLOR_GRAY)
    , middle_connector(parent, middle_connectors_rect)
    , left_gcode_texts(make_left_gcode_text(std::make_index_sequence<max_item_rows>(), parent, left_gcode_label_buffers, gcode_info, drawing_nozzles))
    , right_phys_texts(make_right_phys_text(std::make_index_sequence<max_item_rows>(), parent, right_phys_label_buffers, drawing_nozzles))
    , left_gcode_colors(make_left_gcode_color(std::make_index_sequence<max_item_rows>(), parent, gcode_info))
    , left_gcode_icons(make_left_gcode_icon(std::make_index_sequence<max_item_rows>(), parent))
    , right_phys_icons(make_right_phys_icon(std::make_index_sequence<max_item_rows>(), parent))
    , bottom_guide(parent, bottom_guide_rect, is_multiline::no, is_closed_on_click_t::no, {})
    , bottom_icon(parent, bottom_icon_rect, nullptr)
    , bottom_radio(parent, bottom_radio_rect, responses_with_print)
    , gcode(gcode_info) {

    bottom_guide.SetTextColor(Color::from_raw(0x00CCCCCC));

    for (auto &txt : left_gcode_texts) {
        txt.SetRoundCorners();
        txt.SetAlignment(Align_t::LeftCenter());
    }

    for (auto &txt : right_phys_texts) {
        txt.SetRoundCorners();
        txt.SetAlignment(Align_t::LeftCenter());
    }

    for (auto &col : left_gcode_colors) {
        col.SetRoundCorners();
    }

    // Setup idx_to_real array with values

    // first set them to have default 1-1 2-2 order
    std::iota(std::begin(left_gcode_idx_to_real), std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount(), 0); // default normal order
    std::iota(std::begin(right_phys_idx_to_real), std::begin(right_phys_idx_to_real) + get_num_of_enabled_tools(), 0); // default normal order

    // Then skip some numbers (and increment the rest) if a tool is missing (ie dwarf 1 is not connected)
    for (int left_idx = 0; left_idx < gcode.UsedExtrudersCount(); ++left_idx) {
        for (size_t current_real = left_gcode_idx_to_real[left_idx]; current_real < std::min<size_t>(max_item_rows, EXTRUDERS); ++current_real) {
            // loop until we find a valid gcode
            if (gcode_info.get_extruder_info(current_real).used()) {
                // increment by 1 from this offset to the rest of the array
                std::iota(std::begin(left_gcode_idx_to_real) + left_idx, std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount(), current_real);
                break; // go to fill the next gcode
            }
        }
    }

    for (int right_idx = 0; right_idx < get_num_of_enabled_tools(); ++right_idx) {
        for (size_t current_real = right_phys_idx_to_real[right_idx]; current_real < std::min<size_t>(max_item_rows, EXTRUDERS); ++current_real) {
            // loop until we find a valid physical tool
            if (is_tool_enabled(current_real)) {
                // increment by 1 from this offset to the rest of the array
                std::iota(std::begin(right_phys_idx_to_real) + right_idx, std::begin(right_phys_idx_to_real) + get_num_of_enabled_tools(), current_real);
                break; // go to fill the next physical tool
            }
        }
    }

    auto fill_end_of_array_with_missing_numbers = [](std::array<uint8_t, max_item_rows> &arr, size_t first_empty_idx) {
        size_t idx_array_pos = first_empty_idx;
        for (size_t attempted_assignment = 0; attempted_assignment < std::size(arr); ++attempted_assignment) {
            if (std::find(std::begin(arr), std::begin(arr) + first_empty_idx, attempted_assignment) != std::begin(arr) + first_empty_idx) {
                continue;
            }
            arr[idx_array_pos++] = attempted_assignment;
        }
    };

    // finally make sure that rest of the idx_to_real arrays are filled with inactive gcode/physical tools (ie if we have dwarfs 1,3 , the rest of the array should be 2,4,5)
    fill_end_of_array_with_missing_numbers(left_gcode_idx_to_real, gcode.UsedExtrudersCount());
    fill_end_of_array_with_missing_numbers(right_phys_idx_to_real, get_num_of_enabled_tools());

    std::iota(std::begin(left_gcode_pos_to_real), std::end(left_gcode_pos_to_real), 0); // default order with spaces
    std::iota(std::begin(right_phys_pos_to_real), std::end(right_phys_pos_to_real), 0); // default order with spaces

    if (tool_mapper.is_enabled()) {
        // Marlin contains valid tool mapping - probably preset by Connect. Take that one as our initial input.
        mapper = tool_mapper;
        joiner = spool_join;
    } else {
        // setup mapper to be 1-1, 2-2, but only for each gcode we're trying to assign (unassign the rest)
        mapper.reset(); // default assignment is 1-1, 2-2
        for (size_t i = gcode.UsedExtrudersCount(); i < std::size(left_gcode_idx_to_real); ++i) {
            mapper.set_unassigned(left_gcode_idx_to_real[i]);
        }
        // also unassign when the right side is not available
        for (size_t i = get_num_of_enabled_tools(); i < std::size(right_phys_idx_to_real); ++i) {
            mapper.set_unassigned(right_phys_idx_to_real[i]);
        }
        mapper.set_enable(true);
    }

    bottom_guide.SetAlignment(Align_t::Center());

    // setup middle connector points (positions) - should be in the middle of each row
    window_line_connector::PointsT middle_points;
    middle_points.fill(window_line_connector::unassigned_value);
    for (size_t i = 0; i < max_item_rows; ++i) {
        middle_points[i] = row_first_item + text_row_separation * i // row
            + text_height / 2 // center of text
            - connector_line_height / 2; // offset up to center the line
    }

    middle_connector.set_points(middle_points);
    middle_connector.set_line_thickness(connector_line_height);

    // draw first state
    update_shown_state();

    last_left_real = left_gcode_idx_to_real[0];
    go_left(); // handle first state
}

void ToolsMappingBody::Hide() {
    left_header.Hide();
    right_header.Hide();
    left_line.Hide();
    right_line.Hide();
    for (auto &left_fil : left_gcode_texts) {
        left_fil.Hide();
    }
    for (auto &right_tool : right_phys_texts) {
        right_tool.Hide();
    }
    for (auto &left_color : left_gcode_colors) {
        left_color.Hide();
    }
    for (auto &icon : left_gcode_icons) {
        icon.Hide();
    }
    for (auto &icon : right_phys_icons) {
        icon.Hide();
    }
    bottom_guide.Hide();
    bottom_icon.Hide();
    bottom_radio.Hide();
    middle_connector.Hide();
    window_t::Hide();
}

void ToolsMappingBody::Show() {
    left_header.Show();
    right_header.Show();
    left_line.Show();
    right_line.Show();
    for (size_t i = 0; i < std::size(left_gcode_texts); ++i) {
        if (gcode.get_extruder_info(i).used()) {
            left_gcode_texts[i].Show();
            if (gcode.get_extruder_info(i).extruder_colour.has_value()) {
                left_gcode_colors[i].Show();
            }
        }
    }

    for (size_t i = 0; i < std::size(right_phys_texts); ++i) {
        if (is_tool_enabled(i)) {
            right_phys_texts[i].Show();
        }
    }
    for (auto &icon : left_gcode_icons) {
        icon.Show();
    }
    for (auto &icon : right_phys_icons) {
        icon.Show();
    }

    bottom_guide.Show();
    bottom_icon.Show();
    bottom_radio.Show();
    middle_connector.Show();
    window_t::Show();
}

void ToolsMappingBody::Invalidate() {
    left_header.Invalidate();
    right_header.Invalidate();
    left_line.Invalidate();
    right_line.Invalidate();
    for (auto &left_fil : left_gcode_texts) {
        left_fil.Invalidate();
    }
    for (auto &right_tool : right_phys_texts) {
        right_tool.Invalidate();
    }
    for (auto &left_color : left_gcode_colors) {
        left_color.Invalidate();
    }
    for (auto &icon : left_gcode_icons) {
        icon.Invalidate();
    }
    for (auto &icon : right_phys_icons) {
        icon.Invalidate();
    }
    bottom_guide.Invalidate();
    bottom_icon.Invalidate();
    bottom_radio.Invalidate();
    middle_connector.Invalidate();
    window_t::Invalidate();
}

void ToolsMappingBody::Draw() {
    left_header.Draw();
    right_header.Draw();
    left_line.Draw();
    right_line.Draw();
    for (auto &left_fil : left_gcode_texts) {
        left_fil.Draw();
    }
    for (auto &right_tool : right_phys_texts) {
        right_tool.Draw();
    }
    for (auto &left_color : left_gcode_colors) {
        left_color.Draw();
    }
    for (auto &icon : left_gcode_icons) {
        icon.Draw();
    }
    for (auto &icon : right_phys_icons) {
        icon.Draw();
    }
    bottom_guide.Draw();
    bottom_icon.Draw();
    bottom_radio.Draw();
    middle_connector.Draw();
    window_t::Draw();
}

void ToolsMappingBody::go_left() {
    // if all gcode tools are reasonably mapped, go to done state to allow one-click-through
    if (are_all_gcode_tools_mapped()) {
        bottom_radio.Change(responses_with_print);
        left_line.SetProgressPercent(0.f);
        right_line.SetProgressPercent(0.f);
        current_idx = gcode.UsedExtrudersCount() + print_response_idx;
        set_radio_idx(bottom_radio, print_response_idx);
        state = State::done;
    } else {
        // go to left
        bottom_radio.Change(responses_no_print);
        left_line.SetProgressPercent(100.f);
        right_line.SetProgressPercent(0.f);
        auto idx = std::distance(std::begin(left_gcode_idx_to_real), std::find(std::begin(left_gcode_idx_to_real), std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount(), last_left_real));
        assert(idx >= 0 && idx < gcode.UsedExtrudersCount());
        current_idx = idx;
        set_hovered(left_gcode_texts[left_gcode_idx_to_real[current_idx]], &left_gcode_colors[left_gcode_idx_to_real[current_idx]]);
        disable_radio(bottom_radio);
        state = State::left;
    }

    update_bottom_guide();
#if PRINTER_IS_PRUSA_XL()
    update_dwarf_lights();
#endif
    Invalidate();
}

void ToolsMappingBody::go_right() {
    bottom_radio.Change(responses_no_print);
    left_line.SetProgressPercent(0.f);
    right_line.SetProgressPercent(100.f);
    if (auto real_physical = mapper.to_physical(last_left_real);
        real_physical == ToolMapper::NO_TOOL_MAPPED) {
        // if left has nothing assigned on the right
        current_idx = 0; // 0 in case all tools are assigned
        for (size_t i = 0; i < get_num_of_enabled_tools(); ++i) {
            // need to iterate from 0 to properly break on it if unassigned
            if (mapper.to_gcode(right_phys_idx_to_real[i]) == ToolMapper::NO_TOOL_MAPPED
                && joiner.get_first_spool_1_from_chain(right_phys_idx_to_real[i]) == right_phys_idx_to_real[i]) { // if this tool is unassigned
                current_idx = i;
                break;
            }
        }
    } else {
        // left has something assigned on the right
        auto right_idx = std::distance(std::begin(right_phys_idx_to_real), std::ranges::find(right_phys_idx_to_real, real_physical));
        assert(right_idx >= 0 && right_idx < get_num_of_enabled_tools());
        current_idx = right_idx;
    }
    set_hovered(right_phys_texts[right_phys_idx_to_real[current_idx]], nullptr);
    disable_radio(bottom_radio);

    state = State::right;
    update_bottom_guide();
#if PRINTER_IS_PRUSA_XL()
    update_dwarf_lights();
#endif
    Invalidate();
}

uint8_t ToolsMappingBody::get_cnt_current_items() {
    const size_t cnt_left_items = gcode.UsedExtrudersCount();
    const size_t cnt_right_items = get_num_of_enabled_tools();
    return state == State::right ? cnt_right_items : cnt_left_items;
}

bool ToolsMappingBody::are_all_gcode_tools_mapped() const {
    for (int i = 0; i < gcode.UsedExtrudersCount(); ++i) {
        if (mapper.to_physical(left_gcode_idx_to_real[i]) == ToolMapper::NO_TOOL_MAPPED) {
            return false;
        }
    }
    return true;
}

MultiFilamentChangeConfig ToolsMappingBody::build_changeall_config() {
    MultiFilamentChangeConfig result;

    for (size_t idx = 0; idx < get_num_of_enabled_tools(); ++idx) {
        const auto real_phys = right_phys_idx_to_real[idx];
        const auto real_mapped_gcode = tools_mapping::to_gcode_tool_custom(mapper, joiner, real_phys);

        // Not assigned -> keep as 'don't change'
        if (real_mapped_gcode == tools_mapping::no_tool) {
            continue;
        }

        auto &config = result[real_phys];
        assert(gcode.get_extruder_info(real_mapped_gcode).used()); // otherwise bug in mapping
        config.color = gcode.get_extruder_info(real_mapped_gcode).extruder_colour;

        const auto &opt_name = gcode.get_extruder_info(real_mapped_gcode).filament_name;
        if (!opt_name.has_value()) {
            continue;
        }

        // only preselect if we don't have it already
        if (config_store().get_filament_type(real_phys).matches(opt_name.value().data())) {
            continue;
        }

        config.action = multi_filament_change::Action::change;

        // We're loading a new filament, do not fallback into ad-hoc one -> extruder_index = std::nullopt
        config.new_filament = FilamentType::from_name(opt_name.value().data());
    }

    return result;
}

void ToolsMappingBody::refresh_physical_tool_filament_labels() {
    for (const auto &real : right_phys_pos_to_real) {
        print_right_tool_into_buffer(real, right_phys_label_buffers, drawing_nozzles);
        right_phys_texts[real].SetText(string_view_utf8::MakeRAM(right_phys_label_buffers[real].data()));
    }
}

void ToolsMappingBody::update_bottom_guide() {
    // precondition: icons are properly updated
    static constexpr const char *left_pre_translated = N_("Please select a filament."); // Currently unreachable, always overriden by unassigned gcodes
    static constexpr const char *right_pre_translated = N_("Ready to print");
    static constexpr const char *done_pre_translated = N_("Please assign a tool to the filament");

    static constexpr const char *unassigned_gcodes_pre_translated = N_("Unassigned G-Code filament(s)");
    static constexpr const char *unloaded_tools_pre_translated = N_("Assigned tool(s) without filament");
    static constexpr const char *mismatched_nozzles_pre_translated = N_("Mismatching nozzle diameters");
#if not HAS_MMU2()
    static constexpr const char *mismatched_filaments_pre_translated = N_("Mismatching filament types");
#endif

    string_view_utf8 strview;

    auto print_alert_part_of_guide = [&](const char *state_text, const img::Resource *img) {
        strview = _(state_text);
        bottom_icon.SetRes(img);
        size_t cur_strlen = strview.computeNumUtf8Chars();
        int16_t left_pos = (GuiDefaults::ScreenWidth - (width(Font::normal) + 1) * (cur_strlen + 1) - alert_icon_size) / 2; // make the pos to be on the left of the text (+ one added space to the left of the text)
        Rect16 new_icon_rect = bottom_icon_rect + Rect16::X_t { static_cast<int16_t>(left_pos) };
        bottom_icon.SetRect(static_cast<Rect16>(new_icon_rect));
        bottom_icon.Show();
        bottom_icon.Invalidate();
    };

    if (num_unassigned_gcodes > 0) {
        print_alert_part_of_guide(unassigned_gcodes_pre_translated, unassigned_filament_icon);
    } else if (num_unloaded_tools > 0) {
        print_alert_part_of_guide(unloaded_tools_pre_translated, unloaded_tools_icon);
    } else if (num_mismatched_nozzles > 0) {
        print_alert_part_of_guide(mismatched_nozzles_pre_translated, mismatched_nozzles_icon);
#if not HAS_MMU2()
    } else if (num_mismatched_filaments > 0) {
        // disabled for MMU upon request from Content
        print_alert_part_of_guide(mismatched_filaments_pre_translated, mismatched_filaments_icon);
#endif
    } else {
        bottom_icon.Hide();
        bottom_icon.Invalidate();
        switch (state) {
        case State::left:
            strview = _(left_pre_translated);
            break;
        case State::done:
            strview = _(right_pre_translated);
            break;
        case State::right:
            strview = _(done_pre_translated);
            break;
        default:
            assert(false); // invalid state
            break;
        }
    }

    bottom_guide.SetText(strview);
    bottom_guide.Invalidate();
}

void ToolsMappingBody::update_shown_state_after_scroll(uint8_t previous_idx) {
    // when we're scrolling around, we're only changing highlighting of items or buttons

    if (previous_idx == current_idx) {
        return; // nothing changed
    }

    // precondition: current_idx and previous_idx are valid (in range)
    const size_t cnt_current_items = get_cnt_current_items();

    auto get_real_item = [&](size_t idx) -> window_text_t & {
        return state == State::right ? right_phys_texts[right_phys_idx_to_real[idx]] : left_gcode_texts[left_gcode_idx_to_real[idx]];
    };

    auto get_real_color = [&](size_t idx) -> window_colored_rect * {
        return state == State::right ? nullptr : &left_gcode_colors[left_gcode_idx_to_real[idx]];
    };

    if (previous_idx < cnt_current_items && current_idx < cnt_current_items) {
        // scrolling between items
        window_text_t &previous_item = get_real_item(previous_idx);
        window_text_t &current_item = get_real_item(current_idx);
        set_idle(previous_item, get_real_color(previous_idx));
        set_hovered(current_item, get_real_color(current_idx));
    } else if (previous_idx >= cnt_current_items && current_idx >= cnt_current_items) {
        // scrolling between buttons
        set_radio_idx(bottom_radio, current_idx - cnt_current_items);
    } else {
        // scrolling between an item and a button
        const bool scrolling_from_item_to_button { previous_idx < current_idx };
        const auto idx = scrolling_from_item_to_button ? previous_idx : current_idx;
        window_text_t &item = get_real_item(idx);
        window_colored_rect *color = get_real_color(idx);
        if (scrolling_from_item_to_button) {
            set_idle(item, color);
            set_radio_idx(bottom_radio, 0);
        } else {
            set_hovered(item, color);
            disable_radio(bottom_radio);
        }
    }

#if PRINTER_IS_PRUSA_XL()
    update_dwarf_lights();
#endif
}

void ToolsMappingBody::update_dwarf_lights() {
#if PRINTER_IS_PRUSA_XL()
    HOTEND_LOOP() {
        prusa_toolchanger.getTool(e).set_cheese_led(0, 0); // disable all
    }

    if (current_idx >= get_cnt_current_items()) {
        // all leds are off if within buttons
        return;
    }

    if (state == State::right) {
        prusa_toolchanger.getTool(right_phys_idx_to_real[current_idx]).set_cheese_led(0xff, 0xff);
    } else {
        // find all tools assigned to currently selected left and light them up
        if (auto assigned_tool = mapper.to_physical(left_gcode_idx_to_real[current_idx]); assigned_tool != ToolMapper::NO_TOOL_MAPPED) {
            prusa_toolchanger.getTool(assigned_tool).set_cheese_led(0xff, 0xff);

            // light up all tools that have assigned_tool as their earliest spool_1
            std::for_each(std::begin(right_phys_idx_to_real), std::begin(right_phys_idx_to_real) + get_num_of_enabled_tools(), [&](const auto &right_real) {
                if (joiner.get_first_spool_1_from_chain(right_real) == assigned_tool) {
                    prusa_toolchanger.getTool(right_real).set_cheese_led(0xff, 0xff);
                }
            });
        } // else unassigned and do nothing
    }
#endif // PRINTER_IS_PRUSA_XL()
}

void ToolsMappingBody::update_shown_state() {
    ensure_nicely_ordered();
    update_middle_connectors();
    update_icons();
}

void ToolsMappingBody::update_icons() {
    // precondition: nicely ordered, otherwise the icons will not match
    auto validity = PrintPreview::check_tools_mapping_validity(mapper, joiner, gcode);
    num_unassigned_gcodes = validity.unassigned_gcodes.count();
    num_mismatched_filaments = validity.mismatched_filaments.count();
    num_mismatched_nozzles = validity.mismatched_nozzles.count();
    num_unloaded_tools = validity.unloaded_tools.count();

    for (size_t real_gcode = 0; real_gcode < std::size(left_gcode_icons); ++real_gcode) {
        if (validity.unassigned_gcodes.test(real_gcode)) {
            left_gcode_icons[real_gcode].SetRes(unassigned_filament_icon);
        } else {
            left_gcode_icons[real_gcode].SetRes(nullptr);
        }
    }

    for (size_t real_physical = 0; real_physical < std::size(right_phys_icons); ++real_physical) {
        if (validity.unloaded_tools.test(real_physical)) {
            right_phys_icons[real_physical].SetRes(unloaded_tools_icon);
        } else if (validity.mismatched_nozzles.test(real_physical)) {
            right_phys_icons[real_physical].SetRes(mismatched_nozzles_icon);
#if not HAS_MMU2()
        } else if (validity.mismatched_filaments.test(real_physical)) {
            right_phys_icons[real_physical].SetRes(mismatched_filaments_icon);
#endif
        } else {
            right_phys_icons[real_physical].SetRes(nullptr);
        }
    }
}

void ToolsMappingBody::update_middle_connectors() {
    // precondition: nicely ordered, otherwise the connector will not be nicely updated
    window_line_connector::ParentsT parents;
    parents.fill(window_line_connector::unassigned_value);

    auto assign_to_parent = [&](size_t right_pos, size_t real_left) {
        auto found_left_pos = std::distance(std::begin(left_gcode_pos_to_real), std::ranges::find(left_gcode_pos_to_real, real_left));
        assert(found_left_pos >= 0 && found_left_pos < std::ssize(left_gcode_pos_to_real));
        parents[right_pos] = found_left_pos;
    };

    for (size_t right_idx = 0; right_idx < get_num_of_enabled_tools(); ++right_idx) {

        auto right_pos = std::distance(std::begin(right_phys_pos_to_real), std::ranges::find(right_phys_pos_to_real, right_phys_idx_to_real[right_idx]));
        assert(right_pos >= 0 && right_pos < std::ssize(right_phys_pos_to_real)); // we should be guaranteed that find finds something

        // first check if it's mapped
        if (auto mapped = mapper.to_gcode(right_phys_idx_to_real[right_idx]); mapped != mapper.NO_TOOL_MAPPED) {
            assign_to_parent(right_pos, mapped);
            continue;
        }

        // Check if it's spool joined, find the earliest parent and attach to it
        if (auto earliest_spool_1 = joiner.get_first_spool_1_from_chain(right_phys_idx_to_real[right_idx]); earliest_spool_1 != right_phys_idx_to_real[right_idx]) {
            auto real_gcode = mapper.to_gcode(earliest_spool_1);
            assert(real_gcode != mapper.NO_TOOL_MAPPED); // should be guaranteed, earliest join should always be mapped to something
            assign_to_parent(right_pos, real_gcode);
        }
    }

    middle_connector.set_parents(parents);

    middle_connector.Invalidate();
}

void ToolsMappingBody::adjust_index(int difference) {
    // we're indexing from top to bottom, left to right, +1 means going 'one lower (or right)'

    size_t previous_index { current_idx };
    // add difference to current_idx only if we're not at the respective min/max
    if ((difference > 0
            && ((state == State::left && current_idx + 1 < static_cast<uint8_t>(gcode.UsedExtrudersCount() + responses_count - 1)) // no print
                || (state == State::right && current_idx + 1 < static_cast<uint8_t>(get_num_of_enabled_tools() + responses_count - 1)) // no print
                || (state == State::done && current_idx + 1 < static_cast<uint8_t>(gcode.UsedExtrudersCount() + responses_count))))
        || (difference < 0
            && current_idx > 0)) {
        current_idx += difference;
    } else {
        return; // nothing changed, no need to issue redraw
    }

    update_shown_state_after_scroll(previous_index);
}

void ToolsMappingBody::ensure_nicely_ordered() {
    // Ideal order on the left is: ordered by index, if possible, with possible blank items inbetween (or unassigend gcodes if not possible)
    // Ideal order on the right is: Mapped directly next to the left item, spool joins underneath in join order, unassigned at the end

    const int max_left_blanks { std::ssize(left_gcode_pos_to_real) - gcode.UsedExtrudersCount() };

    // order it so that idx_to_real array is split into two (three) parts -> ordered assigned indices, ordered unassigned indices (+ blanks)
    std::sort(std::begin(left_gcode_idx_to_real), std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount(),
        [&](const auto &lhs_real, const auto &rhs_real) {
            // returns true if lhs should be before rhs

            auto lhs_mapped_to = mapper.to_physical(lhs_real);
            auto rhs_mapped_to = mapper.to_physical(rhs_real);

            if ((lhs_mapped_to == ToolMapper::NO_TOOL_MAPPED && rhs_mapped_to == ToolMapper::NO_TOOL_MAPPED)
                || (lhs_mapped_to != ToolMapper::NO_TOOL_MAPPED && rhs_mapped_to != ToolMapper::NO_TOOL_MAPPED)) {
                // if both are on the same side
                return lhs_real < rhs_real;
            } else if (lhs_mapped_to == ToolMapper::NO_TOOL_MAPPED) {
                // only lhs is unassigned -> rhs should be in the left side, lhs should be on the right
                return false; // lhs should be on the right side of the array
            } else { // if (rhs_mapped_to == ToolMapper::NO_TOOL_MAPPED)
                // only rhs is unassigned -> lhs should be left, rhs should be right
                return true;
            }
        });

    const int first_unassigned_idx {
        [&]() {
            auto found = std::find_if(std::begin(left_gcode_idx_to_real), std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount(), [&](const auto &real) {
                return mapper.to_physical(real) == ToolMapper::NO_TOOL_MAPPED;
            });

            if (found != std::begin(left_gcode_idx_to_real) + gcode.UsedExtrudersCount()) {
                return std::distance(std::begin(left_gcode_idx_to_real), found);
            } else {
                return gcode.UsedExtrudersCount();
            }
        }()
    };

    // precondition: left_idx_to_real is ordered as follows: assigned, unassigned, blanks

    auto move_spool_to_position = [&](size_t new_position, uint8_t real_spool) {
        auto current_position = std::distance(std::begin(right_phys_pos_to_real), std::ranges::find(right_phys_pos_to_real, real_spool));
        assert(current_position >= 0 && current_position < std::ssize(right_phys_pos_to_real));
        if (new_position < std::size(right_phys_pos_to_real)) {
            std::swap(right_phys_pos_to_real[new_position], right_phys_pos_to_real[current_position]);
        } // else leave it where it is
    };

    int used_spool_joins = 0; // how many spool joins we've already 'consumed'
    for (int left_index = 0; left_index < gcode.UsedExtrudersCount(); ++left_index) {
        // looping only through assigned, leave the unassigned in 'any' order
        auto real_left_cur_pos = std::distance(std::begin(left_gcode_pos_to_real), std::ranges::find(left_gcode_pos_to_real, left_gcode_idx_to_real[left_index]));
        assert(real_left_cur_pos >= 0 && real_left_cur_pos < std::ssize(left_gcode_pos_to_real));

        // left_index + used_spool_joins refers to current row (position)

        if (left_index < first_unassigned_idx) {
            assert(left_index + used_spool_joins < std::ssize(left_gcode_pos_to_real)); // assigned should be guaranteed to have a valid row available
            std::swap(left_gcode_pos_to_real[left_index + used_spool_joins], left_gcode_pos_to_real[real_left_cur_pos]); // swap the real to where we want it
        } else {
            // thanks to order precondition, this happens only AFTER all assigned lefts are positioned properly, with blanks (and possibly unassigned lefts) filled into position
            // the only ones that are not yet in their positions are those that weren't used in the assigning

            if (used_spool_joins > max_left_blanks) { // had to place unassigned
                const auto num_reals_moved = used_spool_joins - max_left_blanks;

                if (left_index - first_unassigned_idx < num_reals_moved) {
                    // skip those that have already been put into places
                    continue;
                }

                // note: current row is +max_left_blanks because reals_already_moved are already 'involved' because of the nature of left_index
                std::swap(left_gcode_pos_to_real[left_index + max_left_blanks], left_gcode_pos_to_real[real_left_cur_pos]);
            } else {
                std::swap(left_gcode_pos_to_real[left_index + used_spool_joins], left_gcode_pos_to_real[real_left_cur_pos]);
            }
            continue;
        }

        auto mapped_to = mapper.to_physical(left_gcode_idx_to_real[left_index]);
        assert(mapped_to != ToolMapper::NO_TOOL_MAPPED); // should be handled previously
        if (mapped_to == ToolMapper::NO_TOOL_MAPPED) {
            continue;
        }
        move_spool_to_position(left_index + used_spool_joins, mapped_to);
        auto followup_spool { joiner.get_spool_2(mapped_to) };
        while (followup_spool.has_value()) { // while the spool more joins to it
            // left blanks will be swapped here automagically through swapping the important pieces to places
            ++used_spool_joins;

            if (used_spool_joins > max_left_blanks) {
                // if we don't have a blank, we need to grab the first unassigned and swap it here
                int unassigned_idx = first_unassigned_idx + used_spool_joins - max_left_blanks - 1;
                assert(unassigned_idx < std::ssize(left_gcode_idx_to_real));

                auto unassigned_left_cur_pos = std::distance(std::begin(left_gcode_pos_to_real),
                    std::ranges::find(left_gcode_pos_to_real, left_gcode_idx_to_real[unassigned_idx]));
                assert(unassigned_left_cur_pos >= 0 && unassigned_left_cur_pos < std::ssize(left_gcode_pos_to_real));
                std::swap(left_gcode_pos_to_real[left_index + used_spool_joins], left_gcode_pos_to_real[unassigned_left_cur_pos]);
            } else {
                // if we have a blank, swap it here
                int blank_idx = std::size(left_gcode_pos_to_real) - max_left_blanks + used_spool_joins - 1;
                assert(blank_idx < std::ssize(left_gcode_idx_to_real));
                auto blank_left_cur_pos = std::distance(std::begin(left_gcode_pos_to_real),
                    std::ranges::find(left_gcode_pos_to_real, left_gcode_idx_to_real[blank_idx]));
                assert(blank_left_cur_pos >= 0 && blank_left_cur_pos < std::ssize(left_gcode_pos_to_real));

                std::swap(left_gcode_pos_to_real[left_index + used_spool_joins], left_gcode_pos_to_real[blank_left_cur_pos]);
            }

            move_spool_to_position(left_index + used_spool_joins, followup_spool.value());
            followup_spool = joiner.get_spool_2(followup_spool.value());
        }
    }

    // left/right_pos_to_real should now be updated. Need to update the rects and idx_to_reals

    auto order_idx_arr_based_on_pos = [](const auto &pos_to_real, auto &idx_to_real, size_t num_items) {
        size_t cur_idx { 0 };
        for (size_t pos = 0; pos < std::size(pos_to_real); ++pos) {
            if (auto found = std::find(std::begin(idx_to_real), std::begin(idx_to_real) + num_items, pos_to_real[pos]);
                found != std::begin(idx_to_real) + num_items) {
                auto found_at = std::distance(std::begin(idx_to_real), found);
                assert(found_at >= 0 && found_at < std::ssize(idx_to_real));

                // we found this position in idx_to_real, so we to make sure it's in proper place
                std::swap(idx_to_real[found_at], idx_to_real[cur_idx]); // swap the found number from where it was found to where it belongs
                ++cur_idx;
            }
            // else it's not in idx_to_real, so it's a 'blank'
        }
    };

    order_idx_arr_based_on_pos(right_phys_pos_to_real, right_phys_idx_to_real, get_num_of_enabled_tools());
    order_idx_arr_based_on_pos(left_gcode_pos_to_real, left_gcode_idx_to_real, gcode.UsedExtrudersCount());

    // update rects

    for (size_t current_pos = 0; current_pos < std::size(left_gcode_pos_to_real); ++current_pos) {
        left_gcode_texts[left_gcode_pos_to_real[current_pos]].SetRect(get_left_gcode_rect(current_pos));
        left_gcode_colors[left_gcode_pos_to_real[current_pos]].SetRect(get_left_gcode_color_rect(current_pos));
        left_gcode_icons[left_gcode_pos_to_real[current_pos]].SetRect(get_left_gcode_icon_rect(current_pos));
    }

    for (size_t current_pos = 0; current_pos < std::size(right_phys_pos_to_real); ++current_pos) {
        right_phys_texts[right_phys_pos_to_real[current_pos]].SetRect(get_right_phys_rect(current_pos));
        right_phys_icons[right_phys_pos_to_real[current_pos]].SetRect(get_right_phys_icon_rect(current_pos));
    }

    Invalidate();
}

void ToolsMappingBody::handle_right_steal() {
    auto real_left_has_this_mapped = mapper.to_gcode(right_phys_idx_to_real[current_idx]);
    if (real_left_has_this_mapped != ToolMapper::NO_TOOL_MAPPED) { // if this right was directly mapped to something left
        auto followup_spool { joiner.get_spool_2(right_phys_idx_to_real[current_idx]) };
        if (followup_spool.has_value()) { // and there is a join to this already
            mapper.set_mapping(real_left_has_this_mapped, followup_spool.value());
        } else {
            mapper.set_unassigned(real_left_has_this_mapped);
        }
    }
    joiner.reroute_joins_containing(right_phys_idx_to_real[current_idx]); // reroute all joins from this right
}

void ToolsMappingBody::handle_item_click() {
    // precondition: current_idx is valid

    switch (state) {
    case State::done:
    case State::left: {
        set_selected(left_gcode_texts[left_gcode_idx_to_real[current_idx]], &left_gcode_colors[left_gcode_idx_to_real[current_idx]]);
        last_left_idx = current_idx;
        last_left_real = left_gcode_idx_to_real[current_idx];
        go_right();
        break;
    }
    case State::right: {
        if (auto real_right_mapped_to_last_left = mapper.to_physical(left_gcode_idx_to_real[last_left_idx]);
            real_right_mapped_to_last_left == ToolMapper::NO_TOOL_MAPPED) { // if this left is unassigned
            handle_right_steal();
            [[maybe_unused]] auto rc = mapper.set_mapping(left_gcode_idx_to_real[last_left_idx], right_phys_idx_to_real[current_idx]);
            assert(rc);
        } else if (real_right_mapped_to_last_left == right_phys_idx_to_real[current_idx]) { // trying to assign to oneself again
            // ask about unassign

            auto resp = tools_mapping_box(querying_user, _("This tool is already assigned to this filament."), { Response::Back, Response::Remove }, 1);
            if (resp == Response::Back) { // do nothing, back to selection
                return;
            }
            handle_right_steal();
        } else {
            const auto resp = [&]() -> Response {
                if (real_right_mapped_to_last_left == joiner.get_first_spool_1_from_chain(right_phys_idx_to_real[current_idx])) {
                    // if clicking on right that is spool joined to this left already
                    return tools_mapping_box(querying_user, _("This tool is already spool joined to this filament.\n\nDo you want to REPLACE it as first or REMOVE it from spool join?"), { Response::Back, Response::Remove, Response::Replace }, 2);
                } else {
                    return tools_mapping_box(querying_user, _("This filament already has a tool assigned.\n\nDo you want to REPLACE the assigned tool with the selected tool or add the selected tool for the SPOOL JOIN functionality?"), { Response::Back, Response::SpoolJoin, Response::Replace }, 2);
                }
            }();

            if (resp == Response::Back) { // do nothing, back to selection
                return;
            }

            handle_right_steal();

            if (resp == Response::SpoolJoin) {
                joiner.add_join(real_right_mapped_to_last_left, right_phys_idx_to_real[current_idx]);
            } else if (resp == Response::Replace) {
                // can be replacing a part of chain to be on top, so remove whole chain and start from zero
                // handle_right_steal() would have removed one link from a chain, even if it was this one
                // need to make sure that the chain that was mapped to this left is completely removed before assigning to it
                auto leftover_chain_real_right = mapper.to_physical(left_gcode_idx_to_real[last_left_idx]);
                if (leftover_chain_real_right != ToolMapper::NO_TOOL_MAPPED) {
                    joiner.remove_join_chain_containing(leftover_chain_real_right);
                }
                mapper.set_mapping(left_gcode_idx_to_real[last_left_idx], right_phys_idx_to_real[current_idx]);
            } else if (resp == Response::Remove) {
                // done, just remove it
            } else {
                assert(false);
            }
        }

        set_idle(right_phys_texts[right_phys_idx_to_real[current_idx]], nullptr);
        set_idle(left_gcode_texts[left_gcode_idx_to_real[last_left_idx]], &left_gcode_colors[left_gcode_idx_to_real[last_left_idx]]);

        update_shown_state();
        go_left();
        break;
    }
    }
}

void ToolsMappingBody::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    if (querying_user) {
        return;
    }

    switch (event) {

    case GUI_event_t::CLICK: {
        const size_t cnt_current_items = get_cnt_current_items();

        if (current_idx < cnt_current_items) {
            handle_item_click();
            break;
        }

        auto response = ClientResponses::GetResponse(preview_phase, current_idx - cnt_current_items);

        // Back in right state undoes going to right
        if (response == Response::Back && state == State::right) {
            set_idle(left_gcode_texts[left_gcode_idx_to_real[last_left_idx]], &left_gcode_colors[left_gcode_idx_to_real[last_left_idx]]);
            go_left();
            return;
        } else if (response == Response::PRINT || response == Response::Print) {
            string_view_utf8 warning_text {};
            bool disable_fs { false };

            // unassigned gcodes doesn't allow clicking print
            if (num_unloaded_tools > 0) {
                disable_fs = true;
                warning_text = _("There are printing tools with no filament loaded, this could ruin the print.\nDisable filament sensor and print anyway?");
#if not HAS_MMU2()
            } else if (num_mismatched_filaments > 0) {
                // Hide warning about mismatching filament types for MMU prints
                // - it is yet to be decided how shall we set filament types and work with them in the FW.
                // Contrary to the XL, the MMU is rarely used to switch among different filament types
                // in the same print due to filament mixing in the melt zone.
                warning_text = _("Detected mismatching loaded filament types, this could ruin the print.\nPrint anyway?");
#endif
            } else if (num_mismatched_nozzles > 0) {
                warning_text = _("Detected mismatching nozzle diameters, this could ruin the print.\nPrint anyway?");
            }

            if (!warning_text.isNULLSTR()) {
                AutoRestore ar(querying_user, true);

                if (MsgBoxWarning(warning_text,
                        { Response::Back, Response::Yes })
                    == Response::Back) {
                    return; // go back to tools mapping
                } else {
                    if (disable_fs) {
                        FSensors_instance().set_enabled_global(false);
                    }
                    Screens::Access()->Get()->Validate(); // don't redraw tools mapping screen since we're leaving
                }
            }

            // we're leaving this screen successfully, so update marlin accordingly
            tool_mapper = mapper;
            spool_join = joiner;
        } else if (response == Response::Filament) {
            if (DialogChangeAllFilaments::exec(build_changeall_config(), true)) {
                // This was closed while changing filament by print_abort()
                Screens::Access()->Get()->Validate(); // Do not redraw this
                return;
            }
            refresh_physical_tool_filament_labels();
            update_icons();
            update_bottom_guide();
        }

        if (response == Response::Filament) { // handling change filament locally
            if (GetParent()) {
                GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, nullptr);
            }
        } else { // let marlin handle other responses
            event_conversion_union un;
            un.response = response;
            marlin_client::FSM_response(preview_phase, response);
            if (GetParent()) {
                GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
            }
        }

        return;
    }

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT: {
        // First radio button should always be "back"
        assert(bottom_radio.IndexFromResponse(Response::Back) == 0);

        // "Scroll" to the "back" button - first item in the radio ~ cnt_current_items
        const auto previous_idx = current_idx;
        current_idx = get_cnt_current_items();
        update_shown_state_after_scroll(previous_idx);

        // Emulate click
        WindowEvent(sender, GUI_event_t::CLICK, nullptr);
        return;
    }

    case GUI_event_t::TOUCH_CLICK: {
        const auto touch_point = event_conversion_union { .pvoid = param }.point;

        // We've touched the radio -> process that
        if (bottom_radio.get_rect_for_touch().Contain(touch_point)) {
            bottom_radio.WindowEvent(sender, event, param);

            // "Scroll" to the touched radio button
            const auto previous_idx = current_idx;
            current_idx = get_cnt_current_items() + bottom_radio.GetBtnIndex();
            update_shown_state_after_scroll(previous_idx);

            // Emulate click
            WindowEvent(sender, GUI_event_t::CLICK, nullptr);
            return;
        }

        // Check left items
        for (auto begin = left_gcode_texts.cbegin(), it = begin, end = begin + gcode.UsedExtrudersCount(); it != end; it++) {
            const auto &txt = *it;

            if (!txt.get_rect_for_touch().Contain(touch_point)) {
                continue;
            }

            // If we've touched the left item and are in the "right" state, go back to the left state
            if (state == State::right) {
                // Execute "back" behavior by emulating swipe left
                WindowEvent(sender, GUI_event_t::TOUCH_SWIPE_LEFT, nullptr);
                return;
            }

            // Otherwise, select the appropriate item and emulate click
            const auto previous_idx = current_idx;
            current_idx = left_gcode_pos_to_real[it - begin];
            update_shown_state_after_scroll(previous_idx);
            WindowEvent(sender, GUI_event_t::CLICK, nullptr);

            return;
        }

        // Check right side items
        if (state == State::right) {
            for (auto begin = right_phys_texts.cbegin(), it = begin, end = begin + get_num_of_enabled_tools(); it != end; it++) {
                const auto &txt = *it;

                if (!txt.get_rect_for_touch().Contain(touch_point)) {
                    continue;
                }

                // Select the appropriate item and emulate click
                const auto previous_idx = current_idx;
                current_idx = right_phys_pos_to_real[it - begin];
                update_shown_state_after_scroll(previous_idx);
                WindowEvent(sender, GUI_event_t::CLICK, nullptr);

                return;
            }
        }

        break;
    }

    case GUI_event_t::ENC_UP:
        adjust_index(1);
        return;

    case GUI_event_t::ENC_DN:
        adjust_index(-1);
        return;

    default:
        break;
    }
}
