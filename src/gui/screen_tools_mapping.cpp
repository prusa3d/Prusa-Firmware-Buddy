#include "screen_tools_mapping.hpp"
#include <log.h>
#include <marlin_client.hpp>
#include <module/prusa/toolchanger.h>
#include <string.h>
#include <window_msgbox.hpp>

namespace {

static_assert(window_line_connector::max_one_side_points >= ToolsMappingBody::max_item_rows, "Can't fit all rows into connector");

constexpr size_t text_side_margin { 25 };
constexpr size_t line_side_margin { 15 };
constexpr size_t middle_connector_margin { 10 };
constexpr size_t middle_zone_width { 40 };
constexpr size_t text_height { 22 };
constexpr size_t line_height { 3 };
constexpr size_t text_col_width { (GuiDefaults::ScreenWidth - middle_zone_width) / 2 - 2 * text_side_margin };
constexpr size_t line_col_width { (GuiDefaults::ScreenWidth - middle_zone_width) / 2 - 2 * line_side_margin };

constexpr size_t col_0_text = text_side_margin;
constexpr size_t col_1_text = GuiDefaults::ScreenWidth / 2 + middle_zone_width / 2 + text_side_margin;
constexpr size_t col_0_line = line_side_margin;
constexpr size_t col_1_line = GuiDefaults::ScreenWidth / 2 + middle_zone_width / 2 + line_side_margin;

constexpr size_t text_row_separation { 30 };
constexpr size_t row_headers = 45;
constexpr size_t row_lines = row_headers + 25;
constexpr size_t row_first_item = row_lines + 12;
constexpr size_t row_guide_text = row_first_item + ToolsMappingBody::max_item_rows * text_row_separation;

constexpr Rect16 left_header_rect { col_0_text, row_headers, text_col_width, text_height };
constexpr Rect16 right_header_rect { col_1_text, row_headers, text_col_width, text_height };
constexpr Rect16 left_line_rect { col_0_line, row_lines, line_col_width, line_height };
constexpr Rect16 right_line_rect { col_1_line, row_lines, line_col_width, line_height };

constexpr Rect16 middle_connectors_rect { col_0_text + text_col_width + middle_connector_margin, row_first_item,
    (col_1_text - col_0_text - text_col_width) - 2 * middle_connector_margin, text_height *ToolsMappingBody::max_item_rows };

constexpr Rect16 get_left_filament_rect(size_t idx) {
    return { col_0_text, static_cast<int16_t>(row_first_item + idx * text_row_separation), text_col_width, text_height };
}

constexpr Rect16 get_right_tool_rect(size_t idx) {
    return { col_1_text, static_cast<int16_t>(row_first_item + idx * text_row_separation), text_col_width, text_height };
}

constexpr Rect16 bottom_guide_rect { text_side_margin, row_guide_text, GuiDefaults::ScreenWidth - 2 * text_side_margin, text_height * 2 };
constexpr Rect16 bottom_radio_rect { GuiDefaults::GetButtonRect(GuiDefaults::DialogFrameRect) };

void set_hovered(window_text_t &item) {
    item.SetTextColor(COLOR_BLACK);
    item.SetBackColor(COLOR_WHITE);
}

void set_idle(window_text_t &item) {
    item.SetTextColor(COLOR_WHITE);
    item.SetBackColor(COLOR_BLACK);
}

void set_selected(window_text_t &item) {
    item.SetTextColor(COLOR_ORANGE);
    item.SetBackColor(COLOR_BLACK);
}

// void set_disabled(window_text_t &item) {
//     item.SetTextColor(COLOR_DARK_GRAY);
//     item.SetBackColor(COLOR_BLACK);
// }

void set_radio_idx(RadioButton &radio, size_t idx) {
    radio.EnableDrawingSelected();
    radio.SetBtnIndex(idx);
    radio.Invalidate();
}

void disable_radio(RadioButton &radio) {
    radio.DisableDrawingSelected();
    radio.Invalidate();
}

window_text_t make_right_tool(size_t idx, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers) {

    const auto loaded_filament_type = config_store().get_filament_type(idx);
    const auto loaded_filament_desc = filament::get_description(loaded_filament_type);

    // TODO: filament color (last parameter)
    snprintf(text_buffers[idx].data(), ToolsMappingBody::max_item_text_width, "%d. %s   %s", idx + 1, loaded_filament_desc.name, "");

    window_text_t wtxt { parent, get_right_tool_rect(idx), is_multiline::no, is_closed_on_click_t::no, _(text_buffers[idx].data()) };
    if (!prusa_toolchanger.is_tool_enabled(idx)) {
        wtxt.Hide();
    }
    // TODO: handle if filament not loaded
    // else if (loaded_filament_type == filament::Type::NONE) {
    //     set_disabled(wtxt);
    // }
    return wtxt;
}

template <size_t... Is>
std::array<window_text_t, sizeof...(Is)> make_right_tools(std::index_sequence<Is...>, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_right_tool(Is, parent, text_buffers))... };
}

window_text_t make_left_filament(size_t idx, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, GCodeInfo &gcode) {

    const auto fil_name = gcode.get_extruder_info(idx).filament_name;

    // TODO: filament color (last parameter)
    snprintf(text_buffers[idx].data(), ToolsMappingBody::max_item_text_width, "%d. %s   %s", idx + 1, fil_name.has_value() ? fil_name->data() : "---", "");

    window_text_t wtxt { parent, get_left_filament_rect(idx), is_multiline::no, is_closed_on_click_t::no, _(text_buffers[idx].data()) };
    if (!gcode.get_extruder_info(idx).used()) {
        wtxt.Hide();
    }
    return wtxt;
}

template <size_t... Is>
std::array<window_text_t, sizeof...(Is)> make_left_filaments(std::index_sequence<Is...>, window_t *parent,
    std::array<std::array<char, ToolsMappingBody::max_item_text_width>, ToolsMappingBody::max_item_rows> &text_buffers, GCodeInfo &gcode) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_left_filament(Is, parent, text_buffers, gcode))... };
}

} // namespace

ToolsMappingBody::ToolsMappingBody(window_t *parent, GCodeInfo &gcode_info)
    : AddSuperWindow<window_t>(parent, GuiDefaults::RectScreenNoHeader)
    , left_header(parent, left_header_rect, is_multiline::no, is_closed_on_click_t::no, _("G-Code filaments"))
    , right_header(parent, right_header_rect, is_multiline::no, is_closed_on_click_t::no, _("Printer tools"))
    , left_line(parent, left_line_rect, COLOR_ORANGE, COLOR_GRAY)
    , right_line(parent, right_line_rect, COLOR_ORANGE, COLOR_GRAY)
    , middle_connector(parent, middle_connectors_rect)
    , left_filaments(make_left_filaments(std::make_index_sequence<max_item_rows>(), parent, left_filament_label_buffers, gcode_info))
    , right_tools(make_right_tools(std::make_index_sequence<max_item_rows>(), parent, right_tool_label_buffers))
    , bottom_guide(parent, bottom_guide_rect, is_multiline::yes, is_closed_on_click_t::no, _(""))
    , bottom_radio(parent, bottom_radio_rect, responses)
    , gcode(gcode_info) {

    for (auto &fil : left_filaments) {
        fil.SetRoundCorners(); // doesn't work because window doesn't actually support round corners
    }

    for (auto &tool : right_tools) {
        tool.SetRoundCorners(); // doesn't work because window doesn't actually support round corners
    }

    // Setup idx_to_real array with values

    // first set them to have default 1-1 2-2 order
    std::iota(std::begin(left_log_idx_to_real), std::begin(left_log_idx_to_real) + gcode.UsedExtrudersCount(), 0);                    // default normal order
    std::iota(std::begin(right_phys_idx_to_real), std::begin(right_phys_idx_to_real) + prusa_toolchanger.get_num_enabled_tools(), 0); // default normal order

    // Then skip some numbers (and increment the rest) if a filament/tool is missing (ie dwarf 1 is not connected)
    for (int idx_array_pos = 0; idx_array_pos < gcode.UsedExtrudersCount(); ++idx_array_pos) {
        for (size_t current_offset = left_log_idx_to_real[idx_array_pos]; current_offset < max_item_rows; ++current_offset) {
            // loop until we find a valid filament
            if (gcode_info.get_extruder_info(current_offset).used()) {
                // increment by 1 from this offset to the rest of the array
                std::iota(std::begin(left_log_idx_to_real) + current_offset, std::begin(left_log_idx_to_real) + gcode.UsedExtrudersCount(), current_offset);
                break; // go to fill the next filament
            }
        }
    }

    for (int idx_array_pos = 0; idx_array_pos < prusa_toolchanger.get_num_enabled_tools(); ++idx_array_pos) {
        for (size_t current_offset = right_phys_idx_to_real[idx_array_pos]; current_offset < max_item_rows; ++current_offset) {
            // loop until we find a valid tool
            if (prusa_toolchanger.is_tool_enabled(current_offset)) {
                // increment by 1 from this offset to the rest of the array
                std::iota(std::begin(right_phys_idx_to_real) + current_offset, std::begin(right_phys_idx_to_real) + prusa_toolchanger.get_num_enabled_tools(), current_offset);
                break; // go to fill the next tool
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

    // finally make sure that rest of the idx_to_real arrays are filled with inactive tools/filaments (ie if we have 1-3 dwarfs, the other part should have 2-4-5)
    fill_end_of_array_with_missing_numbers(left_log_idx_to_real, gcode.UsedExtrudersCount());
    fill_end_of_array_with_missing_numbers(right_phys_idx_to_real, prusa_toolchanger.get_num_enabled_tools());

    std::iota(std::begin(left_log_pos_to_real), std::end(left_log_pos_to_real), 0);     // default order with spaces
    std::iota(std::begin(right_phys_pos_to_real), std::end(right_phys_pos_to_real), 0); // default order with spaces

    // setup mapper to be 1-1, 2-2 but only for each filament we're trying to assign (unassign the rest)
    mapper.reset(); // default assignment is 1-1, 2-2
    for (size_t i = gcode.UsedExtrudersCount(); i < std::size(left_log_idx_to_real); ++i) {
        mapper.set_unassigned(left_log_idx_to_real[i]);
    }
    // also unassign when the right side is not available
    for (size_t i = prusa_toolchanger.get_num_enabled_tools(); i < std::size(left_log_idx_to_real); ++i) {
        mapper.set_unassigned(left_log_idx_to_real[i]);
    }
    mapper.set_enable(true);

    bottom_guide.SetAlignment(Align_t::Center());

    // setup middle connector points (positions) - should be in the middle of each row
    window_line_connector::PointsT middle_points;
    middle_points.fill(window_line_connector::unassigned_value);
    for (size_t i = 0; i < max_item_rows; ++i) {
        middle_points[i] = row_first_item + text_height / 2 + text_row_separation * i;
    }

    middle_connector.set_points(middle_points);
    middle_connector.set_line_thickness(line_height);

    // draw first state
    update_middle_connectors();

    // if all filaments are reasonably mapped, go to done state to allow one-click-through
    if (are_all_filaments_mapped()) {
        set_state(State::done);
    } else {
        set_state(State::left);
    }
}

void ToolsMappingBody::Hide() {
    left_header.Hide();
    right_header.Hide();
    left_line.Hide();
    right_line.Hide();
    for (auto &left_fil : left_filaments) {
        left_fil.Hide();
    }
    for (auto &right_tool : right_tools) {
        right_tool.Hide();
    }
    bottom_guide.Hide();
    bottom_radio.Hide();
    middle_connector.Hide();
    window_t::Hide();
}

void ToolsMappingBody::Show() {
    left_header.Show();
    right_header.Show();
    left_line.Show();
    right_line.Show();
    for (size_t i = 0; i < std::size(left_filaments); ++i) {
        if (gcode.get_extruder_info(i).used()) {
            left_filaments[i].Show();
        }
    }

    for (size_t i = 0; i < std::size(right_tools); ++i) {
        if (prusa_toolchanger.is_tool_enabled(i)) {
            right_tools[i].Show();
        }
    }

    bottom_guide.Show();
    bottom_radio.Show();
    middle_connector.Show();
    window_t::Show();
}

void ToolsMappingBody::Invalidate() {
    left_header.Invalidate();
    right_header.Invalidate();
    left_line.Invalidate();
    right_line.Invalidate();
    for (auto &left_fil : left_filaments) {
        left_fil.Invalidate();
    }
    for (auto &right_tool : right_tools) {
        right_tool.Invalidate();
    }
    bottom_guide.Invalidate();
    bottom_radio.Invalidate();
    middle_connector.Invalidate();
    window_t::Invalidate();
}

void ToolsMappingBody::Draw() {
    left_header.Draw();
    right_header.Draw();
    left_line.Draw();
    right_line.Draw();
    for (auto &left_fil : left_filaments) {
        left_fil.Draw();
    }
    for (auto &right_tool : right_tools) {
        right_tool.Draw();
    }
    bottom_guide.Draw();
    bottom_radio.Draw();
    middle_connector.Draw();
    window_t::Draw();
}

void ToolsMappingBody::set_state(State new_state) {
    switch (new_state) {
    case State::left:
        left_line.SetProgressPercent(100.f);
        right_line.SetProgressPercent(0.f);
        bottom_guide.SetText(_("Please select the filament."));
        current_idx = 0; // TODO: first unassigned
        set_hovered(get_real_left_filament(current_idx));
        disable_radio(bottom_radio);
        break;
    case State::right:
        left_line.SetProgressPercent(0.f);
        right_line.SetProgressPercent(100.f);
        bottom_guide.SetText(_("Please assign a tool for the filament."));
        current_idx = 0; // TODO: first unassigned
        set_hovered(get_real_right_tool(current_idx));
        disable_radio(bottom_radio);
        break;
    case State::done:
        left_line.SetProgressPercent(0.f);
        right_line.SetProgressPercent(0.f);
        bottom_guide.SetText(_("All filaments are assigned."));
        current_idx = gcode.UsedExtrudersCount() + print_response_idx;
        set_radio_idx(bottom_radio, print_response_idx);
        break;
    }
    state = new_state;
    Invalidate();
}

window_text_t &ToolsMappingBody::get_real_left_filament(size_t idx) {
    assert(idx < max_item_rows);
    return left_filaments[left_log_idx_to_real[idx]];
}

window_text_t &ToolsMappingBody::get_real_right_tool(size_t idx) {
    assert(idx < max_item_rows);
    return right_tools[right_phys_idx_to_real[idx]];
}

window_text_t &ToolsMappingBody::get_real_item(size_t idx) {
    return state == State::right ? get_real_right_tool(idx) : get_real_left_filament(idx);
}

uint8_t ToolsMappingBody::get_cnt_current_items() {
    const size_t cnt_left_items = gcode.UsedExtrudersCount();
    const size_t cnt_right_items = prusa_toolchanger.get_num_enabled_tools();
    return state == State::right ? cnt_right_items : cnt_left_items;
}

bool ToolsMappingBody::are_all_filaments_mapped() const {
    for (int i = 0; i < gcode.UsedExtrudersCount(); ++i) {
        if (mapper.to_physical(left_log_idx_to_real[i]) == ToolMapper::NO_TOOL_MAPPED) {
            return false;
        }
    }
    return true;
}

void ToolsMappingBody::update_drawn_state_after_scroll(uint8_t previous_idx) {
    // when we're scrolling around, we're only changing highlighting of items or buttons

    if (previous_idx == current_idx) {
        return; // nothing changed
    }

    // precondition: current_idx and previous_idx are valid (in range)
    const size_t cnt_current_items = get_cnt_current_items();

    if (previous_idx < cnt_current_items && current_idx < cnt_current_items) {
        // scrolling between items
        window_text_t &previous_item = get_real_item(previous_idx);
        window_text_t &current_item = get_real_item(current_idx);
        set_idle(previous_item);
        set_hovered(current_item);
    } else if (previous_idx >= cnt_current_items && current_idx >= cnt_current_items) {
        // scrolling between buttons
        set_radio_idx(bottom_radio, current_idx - cnt_current_items);
    } else {
        // scrolling between an item and a button
        const bool scrolling_from_item_to_button { previous_idx < current_idx };
        const auto idx = scrolling_from_item_to_button ? previous_idx : current_idx;
        window_text_t &item = get_real_item(idx);
        if (scrolling_from_item_to_button) {
            set_idle(item);
            set_radio_idx(bottom_radio, 0);
        } else {
            set_hovered(item);
            disable_radio(bottom_radio);
        }
    }
}

void ToolsMappingBody::update_middle_connectors() {

    ensure_nicely_ordered();

    window_line_connector::ParentsT parents;
    parents.fill(window_line_connector::unassigned_value);

    auto assign_to_parent = [&](size_t parent_position, size_t real_left) {
        auto found = std::find(std::begin(left_log_idx_to_real), std::begin(left_log_idx_to_real) + gcode.UsedExtrudersCount(), real_left);
        assert(found != std::begin(left_log_idx_to_real) + gcode.UsedExtrudersCount()); // we should be guaranteed that find finds something
        parents[parent_position] = *found;
    };

    // we're guaranteed that there's (filaments <= tools) -> there's no gaps in right indices
    for (size_t tool_idx = 0; tool_idx < prusa_toolchanger.get_num_enabled_tools(); ++tool_idx) {

        auto right_position = std::distance(std::begin(right_phys_pos_to_real), std::ranges::find(right_phys_pos_to_real, right_phys_idx_to_real[tool_idx]));
        assert(right_position >= 0 && right_position < std::ssize(right_phys_pos_to_real)); // we should be guaranteed that find finds something

        // first check if it's mapped
        if (auto mapped = mapper.to_logical(right_phys_idx_to_real[tool_idx]); mapped != mapper.NO_TOOL_MAPPED) {
            assign_to_parent(right_position, mapped);
            continue;
        }

        // Check if it's spool joined, find the earliest parent and attach to it
        if (auto earliest_spool_1 = joiner.get_earliest_spool_1(right_phys_idx_to_real[tool_idx]); earliest_spool_1 != right_phys_idx_to_real[tool_idx]) {
            assign_to_parent(right_position, mapper.to_logical(earliest_spool_1));
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
            && ((state == State::left && current_idx + 1 < static_cast<uint8_t>(gcode.UsedExtrudersCount() + responses_count - 1))                    // no print
                || (state == State::right && current_idx + 1 < static_cast<uint8_t>(prusa_toolchanger.get_num_enabled_tools() + responses_count - 1)) // no print
                || (state == State::done && current_idx + 1 < static_cast<uint8_t>(gcode.UsedExtrudersCount() + responses_count))))
        || (difference < 0
            && current_idx > 0)) {
        current_idx += difference;
    } else {
        return; // nothing changed, no need to issue redraw
    }

    update_drawn_state_after_scroll(previous_index);
}

void ToolsMappingBody::ensure_nicely_ordered() {
    // There's filaments <= tools, and we want to order stuff nicely
    // Ideal order on the left is: ordered by index, possibly blank items in between
    // Ideal order on the right is: Mapped directly next to the left item, spool joins underneath in join order, unassigned at the end

    // We reorder by assigning to all items rects the way we want them to have

    // redo rects based on potentially new positions

    auto move_spool_to_position = [&](size_t new_position, uint8_t spool) {
        auto mapped_to_cur_pos = std::distance(std::begin(right_phys_pos_to_real), std::ranges::find(right_phys_pos_to_real, spool));
        assert(mapped_to_cur_pos >= 0 && mapped_to_cur_pos < std::ssize(right_phys_pos_to_real));
        if (new_position < std::size(right_phys_pos_to_real)) {
            std::swap(right_phys_pos_to_real[new_position], right_phys_pos_to_real[mapped_to_cur_pos]);
        } // else leave it where it is - this happens if a left filament is unassigned and there's too many joins
    };

    int num_skipped_lefts = 0;

    const int max_left_blanks { std::ssize(left_log_pos_to_real) - gcode.UsedExtrudersCount() };

    for (int left_index = 0; left_index < gcode.UsedExtrudersCount(); ++left_index) {
        //  left_index + num_skipped_lefts = row (position)

        // using left_log_idx_to_real without updating it is ok since left column idx_to_real order should be constant and never change
        auto real_left_cur_pos = std::distance(std::begin(left_log_pos_to_real), std::ranges::find(left_log_pos_to_real, left_log_idx_to_real[left_index]));
        assert(real_left_cur_pos >= 0 && real_left_cur_pos < std::ssize(left_log_pos_to_real));
        if (left_index + num_skipped_lefts < std::ssize(left_log_pos_to_real)                                         // within array
            && num_skipped_lefts < max_left_blanks                                                                    // still have some blank spots that can be used as filler
        ) {
            std::swap(left_log_pos_to_real[left_index + num_skipped_lefts], left_log_pos_to_real[real_left_cur_pos]); // always swap the next index to next position
        }                                                                                                             // if we can't swap it anywhere(too many blanks before), just leave it where it is

        if (auto mapped_to = mapper.to_physical(left_log_idx_to_real[left_index]); mapped_to != ToolMapper::NO_TOOL_MAPPED) {
            // this left is mapped to something -> adjust rights and maybe do blanks on the left

            move_spool_to_position(left_index + num_skipped_lefts, mapped_to);

            auto followup_spool { joiner.get_join_for_tool(mapped_to) };
            while (followup_spool.has_value()) { // while the spool more joins to it
                // left blanks will be swapped here automagically through swapping the important pieces to places
                ++num_skipped_lefts;
                move_spool_to_position(left_index + num_skipped_lefts, followup_spool.value());
                followup_spool = joiner.get_join_for_tool(followup_spool.value());
            }
        }
    }

    // left/right_pos_to_real should now be updated. Need to update the rects and right_idx_to_real (left_idx_to_real is constant and doesn't change)

    // update right_idx_to_real
    size_t cur_right_idx { 0 };
    for (size_t right_pos = 0; right_pos < std::size(right_phys_pos_to_real); ++right_pos) {
        if (auto found = std::find(std::begin(right_phys_idx_to_real), std::begin(right_phys_idx_to_real) + prusa_toolchanger.get_num_enabled_tools(), right_phys_pos_to_real[right_pos]);
            found != std::begin(right_phys_idx_to_real) + prusa_toolchanger.get_num_enabled_tools()) {
            auto found_at = std::distance(std::begin(right_phys_idx_to_real), found);
            assert(found_at >= 0 && found_at < std::ssize(right_phys_idx_to_real));

            // we found this position in idx_to_real, so we to make sure it's in proper place
            std::swap(right_phys_idx_to_real[found_at], right_phys_idx_to_real[cur_right_idx]); // swap the found number from where it was found to where it belongs
            ++cur_right_idx;
        }
        // else it's not in idx_to_real, so it's a 'blank'
    }

    // update rects

    for (size_t current_pos = 0; current_pos < std::size(left_log_pos_to_real); ++current_pos) {
        left_filaments[left_log_pos_to_real[current_pos]].SetRect(get_left_filament_rect(current_pos));
    }

    for (size_t current_pos = 0; current_pos < std::size(right_phys_pos_to_real); ++current_pos) {
        right_tools[right_phys_pos_to_real[current_pos]].SetRect(get_right_tool_rect(current_pos));
    }

    Invalidate();
}

void ToolsMappingBody::handle_right_replacement() {

    auto tool_will_print_real_filament = mapper.to_logical(right_phys_idx_to_real[current_idx]);
    if (tool_will_print_real_filament != ToolMapper::NO_TOOL_MAPPED) { // if this right was mapped to something left
        auto followup_spool { joiner.get_join_for_tool(right_phys_idx_to_real[current_idx]) };
        if (followup_spool.has_value()) {                              // and there is a join to this already
            mapper.set_mapping(tool_will_print_real_filament, followup_spool.value());
        } else {
            mapper.set_unassigned(tool_will_print_real_filament);
        }
    }

    joiner.remove_joins_containing(right_phys_idx_to_real[current_idx]); // remove all joins from this right
}

void ToolsMappingBody::handle_item_click() {
    // precondition: current_idx is valid

    switch (state) {
    case State::done:
    case State::left: {
        set_selected(get_real_left_filament(current_idx));
        last_left_idx = current_idx;
        set_state(State::right);
        break;
    }
    case State::right: {
        if (auto filament_assigned_to_real_tool = mapper.to_physical(left_log_idx_to_real[last_left_idx]);
            filament_assigned_to_real_tool == ToolMapper::NO_TOOL_MAPPED) { // if this left is unassigned
            handle_right_replacement();
            [[maybe_unused]] auto rc = mapper.set_mapping(left_log_idx_to_real[last_left_idx], right_phys_idx_to_real[current_idx]);
            assert(rc);
        } else if (filament_assigned_to_real_tool == right_phys_idx_to_real[current_idx]) { // trying to assign to oneself again
            // Unassign
            handle_right_replacement();
        } else {
            static constexpr PhaseResponses responses = { Response::Back, Response::SpoolJoin, Response::Replace };
            const PhaseTexts labels = { BtnResponse::GetText(responses[0]), BtnResponse::GetText(responses[1]), BtnResponse::GetText(responses[2]), BtnResponse::GetText(responses[3]) };
            MsgBoxBase msgbox(GuiDefaults::DialogFrameRect, responses, 0, &labels, _("A tool is already assigned to this filament.\n\nDo you want to replace it\nor add it as an additional one\nfor the Spool Join functionality?"), is_multiline::yes);
            msgbox.set_text_alignment(Align_t::Center());
            msgbox.MakeBlocking();
            auto resp = msgbox.GetResult();

            if (resp == Response::Back) { // do nothing, back to selection
                return;
            }

            handle_right_replacement();

            if (resp == Response::SpoolJoin) {
                joiner.add_join(filament_assigned_to_real_tool, right_phys_idx_to_real[current_idx]);
            } else if (resp == Response::Replace) {
                mapper.set_mapping(left_log_idx_to_real[last_left_idx], right_phys_idx_to_real[current_idx]);
            } else {
                assert(false);
            }
        }

        set_idle(get_real_right_tool(current_idx));
        set_idle(get_real_left_filament(last_left_idx));

        update_middle_connectors();

        if (are_all_filaments_mapped()) {
            set_state(State::done);
        } else {
            set_state(State::left);
        }
        break;
    }
    }
}

void ToolsMappingBody::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        const size_t cnt_current_items = get_cnt_current_items();

        if (current_idx >= cnt_current_items) { // in buttons

            auto response = ClientResponses::GetResponse(preview_phase, current_idx - cnt_current_items);

            // Back in right state undoes going to right
            if (response == Response::Back && state == State::right) {
                set_idle(get_real_left_filament(last_left_idx));
                set_state(State::left);
                return;
            } else if (response == Response::PRINT) {
                // we're leaving this screen successfully, so update marlin accordingly
                tool_mapper = mapper;
                spool_join = joiner;
            }

            event_conversion_union un;
            un.response = response;
            marlin_client::FSM_response(preview_phase, response);
            if (GetParent()) {
                GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
            }
        } else {
            handle_item_click();
        }
        break;
    }
    case GUI_event_t::ENC_UP:
        adjust_index(1);
        break;
    case GUI_event_t::ENC_DN:
        adjust_index(-1);
        break;
    default:
        break;
    }
}
