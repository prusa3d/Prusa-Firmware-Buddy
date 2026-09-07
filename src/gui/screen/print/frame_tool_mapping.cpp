/// @file
#include "frame_tool_mapping.hpp"

#include <guiconfig/GuiDefaults.hpp>
#include <gcode/gcode_info.hpp>
#include <tools_mapping.hpp>
#include <utils/string_builder.hpp>
#include <display.hpp>
#include <utils/variant_utils.hpp>
#include <module/prusa/tool_mapper.hpp>
#include <module/prusa/spool_join.hpp>
#include <window_msgbox.hpp>
#include <gui/ScreenHandler.hpp>
#include <raii/scope_guard.hpp>
#include <utils/compact_optional.hpp>
#include <img_resources.hpp>
#include <fsm/print_preview_phases.hpp>
#include <multi_filament_change.hpp>
#include <screen_menu_filament_changeall.hpp>
#include <option/has_mmu2.h>
#include <bsod/bsod.h>

namespace screen_tool_mapping {

namespace {

    constexpr const IWindowMenuItem::ColorScheme color_scheme_selected_for_editing {
        .text {
            .focused = COLOR_BRAND,
            .unfocused = GuiDefaults::MenuColorText,
        },
        .back {
            .focused = GuiDefaults::MenuColorFocusedBack,
            .unfocused = COLOR_BRAND,
        },
        .rop = {},
    };

    Response show_msg_box(const string_view_utf8 &msg, PhaseResponses responses, size_t default_button = 0) {
        MsgBoxBase msgbox(GuiDefaults::DialogFrameRect, responses, default_button, msg, is_multiline::yes);
        msgbox.set_text_alignment(Align_t::Center());
        Screens::Access()->gui_loop_until_dialog_closed();
        return msgbox.GetResult();
    }

    void disconnect_virtual_tool(VirtualToolIndex virtual_tool) {
        // We need to update tool mapping if the virtual tool was first in the spool join
        const auto gcode_tool = stdext::get_optional<GcodeToolIndex>(tool_mapper.to_gcode(virtual_tool));
        if (gcode_tool.has_value()) {
            const auto next_in_chain = spool_join.get_spool_2(virtual_tool);
            if (next_in_chain) {
                tool_mapper.set_mapping(*gcode_tool, *next_in_chain);
            } else {
                tool_mapper.set_unassigned(*gcode_tool);
            }
        }

        // Remove the tool from the spool join chain
        spool_join.reroute_joins_containing(virtual_tool);
    }

} // namespace

ColumnItem::ColumnItem(FrameToolMapping &frame, const ToolType type, const uint8_t item_index)
    : frame_(frame)
    , type_(type)
    , item_index_(item_index) {

    extension_width = menu_item_extension_w;

    update();
}

void ColumnItem::update() {
    const auto tool = this->tool();
    const bool is_selected_for_connecting = !std::holds_alternative<NoTool>(tool) && (frame_.selected_tool_for_connecting() == tool);
    const bool is_focusable = frame_.is_item_focusable(type_, item_index_);
    set_color_scheme(
        is_selected_for_connecting ? &color_scheme_selected_for_editing
            : !is_focusable        ? &color_scheme_default_disabled
                                   : &IWindowMenuItem::color_scheme_default);
}

ToolVariant ColumnItem::tool() const {
    switch (type_) {

    case ToolType::gcode_tool:
        if (auto gcode_tool = frame_.items()[item_index_].gcode_tool) {
            return *gcode_tool;
        }
        break;

    case ToolType::virtual_tool:
        if (auto virtual_tool = frame_.items()[item_index_].virtual_tool) {
            return *virtual_tool;
        }
        break;
    }

    return NoTool {};
}

void ColumnItem::click(IWindowMenu &) {
    const ToolVariant this_tool = this->tool();
    if (std::holds_alternative<NoTool>(this_tool)) {
        return;
    }

    const auto currently_selected_tool = frame_.selected_tool_for_connecting();
    const auto other_tool_type = (type_ == ToolType::gcode_tool) ? ToolType::virtual_tool : ToolType::gcode_tool;

    if (currently_selected_tool == this_tool) {
        // We've clicked on the same tool -> unselect
        // TODO perhaps we could offer unlinking here
        frame_.select_tool_for_connecting(NoTool {});

    } else if (currently_selected_tool.index() == this_tool.index() || std::holds_alternative<NoTool>(currently_selected_tool)) {
        // The tool is of the same type as the current one -> change the selection to this tool
        frame_.select_tool_for_connecting(this_tool);

        // Move focus to the first unconnected item on the other menu to streamline routing
        frame_.focus_first_unconnected_tool(other_tool_type);

        // Guide the user to continue connecting
        frame_.update_status_text(ShowGuide::yes);

    } else {
        // The currently selected tool is a different type - we can make a connection
        const GcodeToolIndex gcode_tool = std::get<GcodeToolIndex>(type_ == ToolType::gcode_tool ? this_tool : currently_selected_tool);
        const VirtualToolIndex virtual_tool = std::get<VirtualToolIndex>(type_ == ToolType::virtual_tool ? this_tool : currently_selected_tool);

        const auto virtual_tool_chain_start = spool_join.get_first_spool_1_from_chain(virtual_tool);
        const auto virtual_tool_original_gcode_tool = stdext::get_optional<GcodeToolIndex>(tool_mapper.to_gcode(virtual_tool_chain_start));

        // When exiting, we need to update the frame and reorder items
        ScopeGuard scope_guard = [&] {
            frame_.select_tool_for_connecting(NoTool {});

            // Move focus to the first unconnected item on the gcode side to streamline routing
            frame_.focus_first_unconnected_tool(ToolType::gcode_tool);

            // Hide the guide, show possible errors
            frame_.update_status_text(ShowGuide::no);

            frame_.schedule_mapping_update();
        };

        if (gcode_tool == virtual_tool_original_gcode_tool) {
            // Trying to assign to the same tool
            const auto resp = show_msg_box(_("This tool is already assigned to this filament."), { Response::Cancel, Response::Remove }, 1);
            switch (resp) {

            case Response::Cancel:
                scope_guard.disarm();
                return;

            case Response::Remove:
                disconnect_virtual_tool(virtual_tool);
                return;

            default:
                bsod_unreachable();
            }

        } else {
            // Assigning to a different tool (or not tool assigned before)
            const auto gcode_tool_original_virtual_tool = stdext::get_optional<VirtualToolIndex>(tool_mapper.to_virtual(gcode_tool));
            if (gcode_tool_original_virtual_tool.has_value()) {
                // The gcode tool already has a tool assigned - we offer to either replace or spool join
                const auto resp = show_msg_box(
                    _("This G-Code filament already has a printer tool assigned.\n\nDo you want to REPLACE the connection or add the tool into a SPOOL JOIN, switching to it once the original tool runs out of material?"),
                    { Response::Cancel, Response::SpoolJoin, Response::Replace }, 2);

                switch (resp) {

                case Response::SpoolJoin:
                    disconnect_virtual_tool(virtual_tool);
                    spool_join.add_join(spool_join.get_last_spool_2_from_chain(*gcode_tool_original_virtual_tool), virtual_tool);
                    break;

                case Response::Replace:
                    disconnect_virtual_tool(virtual_tool);
                    spool_join.remove_join_chain_containing(*gcode_tool_original_virtual_tool);
                    tool_mapper.set_mapping(gcode_tool, virtual_tool);
                    break;

                case Response::Cancel:
                    scope_guard.disarm();
                    return;

                default:
                    bsod_unreachable();
                }

            } else {
                // Straightforward, just assign
                tool_mapper.set_mapping(gcode_tool, virtual_tool);
            }
        }
    }
}

void ColumnItem::printExtension(Rect16 extension_rect, Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {

    struct Data {
        string_view_utf8 tool_name;
        CompactOptional<float, NAN> nozzle_diameter;
        FilamentTypeParameters::Name filament_name;
        CompactOptional<Color, COLOR_NONE> color;
    } data;
    StringViewUtf8Parameters<3> tool_name_buf;

    // Determine the data to display
    switch (type_) {

    case ToolType::gcode_tool: {
        const auto gcode_tool = frame_.items()[item_index_].gcode_tool;
        if (!gcode_tool) {
            return;
        }

        const auto &ei = GCodeInfo::getInstance().get_extruder_info(*gcode_tool);

        data = Data {
            .tool_name = gcode_tool->compact_display_name(tool_name_buf),
            .nozzle_diameter = ei.nozzle_diameter,
            .filament_name = ei.filament_name,
            .color = ei.extruder_colour,
        };
        break;
    }

    case ToolType::virtual_tool:
        const auto virtual_tool = frame_.items()[item_index_].virtual_tool;
        if (!virtual_tool) {
            return;
        }

        const auto physical_tool = virtual_tool->to_physical();

        data = Data {
            .tool_name = virtual_tool->compact_display_name(tool_name_buf),
            .nozzle_diameter = CompactOptional<float, NAN> { config_store().get_nozzle_diameter(physical_tool) },
            .filament_name = config_store().get_filament_type(*virtual_tool).parameters().name,
            .color = std::nullopt, // TODO: Support loaded filament colors
        };
        break;
    }

    int16_t x = extension_rect.Left() + 4;

    const auto render_column = [&](const string_view_utf8 &text, uint8_t column_w_chars, bool is_visible = true) {
        const int w = resource_font_size(menu_font).w * column_w_chars;
        if (is_visible) {
            render_text_align(
                Rect16::fromLTWH(x, extension_rect.Top(), w, extension_rect.Height()),
                text,
                menu_font, color_back, color_text, padding_ui8_t {}, text_flags { Align_t::LeftCenter() });
        }
        x += w;
    };

    render_column(data.tool_name, 3);
    render_column(string_view_utf8::MakeRAM(data.filament_name), filament_name_buffer_size);

    {
        ArrayStringBuilder<4> nozzle_dia_buf;
        if (data.nozzle_diameter.has_value()) {
            nozzle_dia_buf.append_float(*data.nozzle_diameter, { .skip_zero_before_dot = true });
        } else {
            nozzle_dia_buf.append_string("--");
        }
        render_column(string_view_utf8::MakeRAM(nozzle_dia_buf.str()), nozzle_dia_buf.array_size, frame_.config().show_nozzle_diameter);
    }

    // Draw filament color
    if (data.color.has_value()) {
        constexpr auto margin = 6;
        constexpr auto padding = 1;

        const auto outer_size = extension_rect.Height() - margin * 2;
        const Rect16 outer_rect = Rect16::fromLTWH(extension_rect.Right() - outer_size, extension_rect.Top() + margin, outer_size, outer_size);
        display::draw_rounded_rect(outer_rect, color_back, COLOR_WHITE, GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);

        const auto inner_size = outer_size - padding * 2;
        const Rect16 inner_rect = Rect16::fromLTWH(outer_rect.Left() + padding, outer_rect.Top() + padding, inner_size, inner_size);
        display::draw_rounded_rect(inner_rect, COLOR_WHITE, *data.color, GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS);
    }
}

ColumnMenu::ColumnMenu(FrameToolMapping &frame, const Rect16 &rect, ToolType type)
    : WindowMenuVirtualSized(&frame, rect, CloseScreenReturnBehavior::no)
    , frame_(frame)
    , type_(type) {
    set_item_height(menu_item_h);
}

int ColumnMenu::item_count() const {
    return frame_.items().size();
}

bool ColumnMenu::is_item_focusable(int index) const {
    return frame_.is_item_focusable(type_, index);
}

void ColumnMenu::setup_item(ItemVariant &variant, int index) {
    variant.emplace<ColumnItem>(frame_, type_, (uint8_t)index);
}

void ColumnMenu::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::SCROLL_CHANGED:
        frame_.update_scroll();
        break;

    default:
        break;
    }

    WindowMenuVirtualSized::windowEvent(sender, event, param);
}

WindowRoutingBoard::WindowRoutingBoard(FrameToolMapping &frame)
    : window_t(&frame, Rect16::fromLTRB(routing_board_l, menu_t, routing_board_r, menu_b))
    , frame_(frame) {}

void WindowRoutingBoard::unconditionalDraw() {
    const auto back_color = GetBackColor();
    display::fill_rect(GetRect(), back_color);

    const auto first_item = frame_.gcode_tools_menu_.scroll_offset();
    const auto end = std::min<int>(frame_.items_.size(), first_item + frame_.gcode_tools_menu_.max_items_on_screen_count());

    auto draw_failed_check = [&](auto failed_check, uint16_t x, uint16_t y) {
        if (!failed_check.has_value()) {
            return;
        }

        const auto icon = buddy::compatibility_checks::compatibility_level_icons[failed_check->meta->evaluate_compatibility()];
        if (!icon) {
            return;
        }

        display::draw_img({ x, y }, *icon, back_color, {});
    };

    for (int i = first_item; i < end; i++) {
        const auto &item = frame_.items_[i];
        const uint16_t y = frame_.gcode_tools_menu_.slot_rect(i - first_item).Top();

        const point_ui16_t center((routing_board_l + routing_board_r) / 2, y + menu_item_h / 2);

        switch (item.connection) {

        case MappingDisplayItem::Connection::main_tool:
            display::draw_line({ routing_board_l, center.y }, { routing_board_r, center.y }, line_color);
            break;

        case MappingDisplayItem::Connection::spool_join: {
            display::draw_line({ center.x, y }, center, line_color);
            display::draw_line(center, { routing_board_r, center.y }, line_color);

            const auto &spool_join_icon = img::spool_16x16;
            display::draw_img(point_ui16(center.x - spool_join_icon.w / 2, y), spool_join_icon, back_color, {});
            break;
        }

        case MappingDisplayItem::Connection::none:
            break;
        }

        if (i < (int)frame_.items_.size() - 1 && frame_.items_[i + 1].connection == MappingDisplayItem::Connection::spool_join) {
            // Draw line down
            display::draw_line(center, point_ui16_t(center.x, y + menu_item_h), line_color);
        }

        if (item.gcode_tool.has_value()) {
            draw_failed_check(frame_.compat_report_.highest_severity_failed_check(*item.gcode_tool), routing_board_l + 4, y + (menu_item_h - failed_check_icon_size) / 2);
        }
        if (item.virtual_tool.has_value()) {
            draw_failed_check(frame_.compat_report_.highest_severity_failed_check(*item.virtual_tool), routing_board_r - failed_check_icon_size - 4, y + (menu_item_h - failed_check_icon_size) / 2);
        }
    }
}

FrameToolMapping::FrameToolMapping(window_frame_t *parent)
    : window_frame_t(parent)
    , gcode_tools_title_(this, Rect16::fromLTWH(0, title_t, menu_w, title_h - title_line_h - 2), is_multiline::no)
    , gcode_tools_title_line_(this, Rect16::fromLTWH(0, title_t + title_h - title_line_h, menu_w, title_line_h), COLOR_GRAY)
    , gcode_tools_menu_(*this, Rect16::fromLTWH(0, menu_t, menu_w, menu_h), ToolType::gcode_tool)
    , virtual_tools_title_(this, Rect16::fromLTWH(right_menu_l, title_t, menu_w, title_h - title_line_h - 2), is_multiline::no)
    , virtual_tools_title_line_(this, Rect16::fromLTWH(right_menu_l, title_t + title_h - title_line_h, menu_w, title_line_h), COLOR_GRAY)
    , virtual_tools_menu_(*this, Rect16::fromLTWH(right_menu_l, menu_t, menu_w, menu_h), ToolType::virtual_tool)
    , routing_board_(*this)
    , scrollbar_(this, Rect16::fromLTWH(right_menu_r, menu_t, GuiDefaults::MenuScrollbarWidth, menu_h), gcode_tools_menu_)
    , bottom_status_(this, Rect16::fromLTRB(16, menu_b, GuiDefaults::ScreenWidth - 16, button_rect.Top()), is_multiline::yes)
    , bottom_buttons_(this, button_rect) //
{
    parent->CaptureNormalWindow(*this);

    for (auto *title : { &gcode_tools_title_, &virtual_tools_title_ }) {
        title->set_font(Font::small);
        title->SetAlignment(Align_t::CenterBottom());
    }

    bottom_status_.set_font(Font::normal);
    bottom_status_.SetAlignment(Align_t::Center());

    bottom_buttons_.Change(PhaseResponses { Response::Abort, Response::Filament, Response::Remap, Response::Print });
    bottom_buttons_.SetFocus();
    bottom_buttons_.SetBtnIndex(3); // Pre-select the print button

    if (!tool_mapper.is_enabled()) {
        // The mapper has not been set up -> set up the default config
        // resets 1-1, 2-2 etc
        tool_mapper.reset();

        // And disable all unused/invalid tools
        for (auto gcode_tool : GcodeToolIndex::all()) {
            auto virtual_tool = stdext::get_optional<VirtualToolIndex>(tool_mapper.to_virtual(gcode_tool));
            if (
                !GCodeInfo::getInstance().get_extruder_info(gcode_tool).used()
                || (virtual_tool.has_value() && !virtual_tool->is_enabled()) //
            ) {
                tool_mapper.set_unassigned(gcode_tool);
            }
        }
        tool_mapper.set_enable(true);
    }

    update_config();
    update_mapping();
    update_scroll();

    select_tool_for_connecting(NoTool {});
}

FrameToolMapping::~FrameToolMapping() {
}

ToolVariant FrameToolMapping::focused_tool() {
    if (auto index = gcode_tools_menu_.focused_item_index()) {
        if (auto tool = items_[*index].gcode_tool) {
            return *tool;
        }

    } else if (auto index = virtual_tools_menu_.focused_item_index()) {
        if (auto tool = items_[*index].virtual_tool) {
            return *tool;
        }
    }

    return NoTool {};
}

void FrameToolMapping::focus_tool(ToolVariant tool) {
    const auto item = tool_item_position(tool);
    if (item.menu) {
        item.menu->SetFocus();
        item.menu->move_focus_to_index(item.index);
    } else {
        gcode_tools_menu_.move_focus_to_index(std::nullopt);
        virtual_tools_menu_.move_focus_to_index(std::nullopt);
    }
}

void FrameToolMapping::focus_first_unconnected_tool(ToolType tool_type) {
    ToolVariant tool_to_focus;
    switch (tool_type) {

    case ToolType::virtual_tool: {
        for (const auto &item : items_) {
            if (auto tool = item.virtual_tool) {
                tool_to_focus = *tool;

                // Break on first unconnected. Otherwise continue so that we have at least something to select
                switch (item.connection) {

                case MappingDisplayItem::Connection::none:
                    goto break_loop_virtual;

                case MappingDisplayItem::Connection::spool_join:
                case MappingDisplayItem::Connection::main_tool:
                    break;
                }
            }
        }
    break_loop_virtual:
        break;
    }

    case ToolType::gcode_tool: {
        for (const auto &item : items_) {
            if (auto tool = item.gcode_tool) {
                tool_to_focus = *tool;

                // Break on first unconnected. Otherwise continue so that we have at least something to select
                switch (item.connection) {

                case MappingDisplayItem::Connection::none:
                case MappingDisplayItem::Connection::spool_join: // gcode_tool is not connected in the spool_join connection
                    goto break_loop_gcode;

                case MappingDisplayItem::Connection::main_tool:
                    break;
                }
            }
        }
    break_loop_gcode:
        break;
    }
    }

    focus_tool(tool_to_focus);
}

FrameToolMapping::ToolItemPosition FrameToolMapping::tool_item_position(ToolVariant tool) {
    ToolItemPosition result;

    match(
        tool, //
        [&](GcodeToolIndex gcode_tool) {
            result = ToolItemPosition { .menu = &gcode_tools_menu_, .index = gcode_tool_positions_[gcode_tool] };
        },
        [&](VirtualToolIndex virtual_tool) {
            result = ToolItemPosition { .menu = &virtual_tools_menu_, .index = virtual_tool_positions_[virtual_tool] };
        },
        [](NoTool) {} //
    );

    if (result.index == invalid_tool_position) {
        result.menu = nullptr;
    }

    return result;
}

ColumnItem *FrameToolMapping::tool_menu_item(ToolVariant tool) {
    const auto pos = tool_item_position(tool);
    return pos.menu ? static_cast<ColumnItem *>(pos.menu->item_at(pos.index)) : nullptr;
}

void FrameToolMapping::select_tool_for_connecting(ToolVariant tool) {
    // Do NOT lazily exit if tool == selected_tool_for_connecting_
    // We need to do this unconditionally in the constructor

    selected_tool_for_connecting_ = tool;

    // Only allow selecting virtual tools if a g-code tool for connecting is selected
    // The workflow for the knob is always fist select gcode tool, then select virtual tool
    // !!! Must be before updating the items, update() changes visuals based on the menu being enabled
    virtual_tools_menu_.set_enabled(std::holds_alternative<GcodeToolIndex>(selected_tool_for_connecting_));

    // Update all items
    for (auto *menu : { &gcode_tools_menu_, &virtual_tools_menu_ }) {
        for (auto &item : menu->buffered_items()) {
            if (auto ci = item.get_if<ColumnItem>()) {
                ci->update();
            }
        }
    }
}

bool FrameToolMapping::is_item_focusable(ToolType type, uint8_t index) const {
    const auto &item = items_[index];

    switch (type) {

    case ToolType::gcode_tool:
        return item.gcode_tool.has_value();

    case ToolType::virtual_tool:
        return item.virtual_tool.has_value() && virtual_tools_menu_.IsEnabled();
    }

    bsod_unreachable();
}

void FrameToolMapping::update_config() {
    config_ = {};

    const auto &gcode_info = GCodeInfo::getInstance();

    CompactOptional<float, NAN> common_nozzle_diameter;

    auto check_mismatch = [](auto &common, auto val) -> bool {
        if constexpr (requires { val.has_value(); }) {
            if (!val.has_value()) {
                return false;
            }
        }

        const auto result = common.has_value() && (common != val);
        common = val;
        return result;
    };

    for (const auto gcode_tool : GcodeToolIndex::all()) {
        const auto &ei = gcode_info.get_extruder_info(gcode_tool);
        if (!ei.used()) {
            continue;
        }

        config_.show_nozzle_diameter |= check_mismatch(common_nozzle_diameter, ei.nozzle_diameter);
    }

    for (const auto virtual_tool : VirtualToolIndex::all().skip_all_disabled()) {
        const auto physical_tool = virtual_tool.to_physical();

        config_.show_nozzle_diameter |= check_mismatch(common_nozzle_diameter, config_store().get_nozzle_diameter(physical_tool));
    }
}

void FrameToolMapping::update_mapping() {
    items_.clear();

    const auto original_focused_tool = this->focused_tool();

    const auto &gci = GCodeInfo::getInstance();

    std::bitset<GcodeToolIndex::count> processed_gcode_tools;
    std::bitset<VirtualToolIndex::count> processed_virtual_tools;

    // First mark all gcode tools that are not used in the gcode at all as processed
    for (const auto gcode_tool : GcodeToolIndex::all()) {
        processed_gcode_tools.set(gcode_tool.to_raw(), !gci.get_extruder_info(gcode_tool).used());
    }

    // Ditto with disabled virtual tools
    for (const auto virtual_tool : VirtualToolIndex::all()) {
        processed_virtual_tools.set(virtual_tool.to_raw(), !virtual_tool.is_enabled());
    }

    // Update the titles
    {
        gcode_tools_title_.SetText(_("G-Code filaments (%i)").formatted(gcode_tools_title_params_, GcodeToolIndex::count - processed_gcode_tools.count()));

#if HAS_MMU2()
        const char *virtual_tool_title = N_("MMU filaments (%i)");
#else
        const char *virtual_tool_title = N_("Printer tools (%i)");
#endif
        virtual_tools_title_.SetText(_(virtual_tool_title).formatted(virtual_tools_title_params_, VirtualToolIndex::count - processed_virtual_tools.count()));
    }

    // Show Actually used GCode tools in order
    for (const auto gcode_tool : GcodeToolIndex::all()) {
        if (processed_gcode_tools.test(gcode_tool.to_raw())) {
            continue;
        }

        const auto base_virtual_tool = stdext::get_optional<VirtualToolIndex>(tool_mapper.to_virtual(gcode_tool));
        if (!base_virtual_tool.has_value()) {
            // The gcode tool is not mapped - will be added after the mapped tools
            // Do NOT mark the gcode_tool as processed
            continue;
        }

        items_.push_back(MappingDisplayItem {
            .gcode_tool = gcode_tool,
            .virtual_tool = base_virtual_tool,
            .connection = MappingDisplayItem::Connection::main_tool,
        });
        processed_gcode_tools.set(gcode_tool.to_raw());
        processed_virtual_tools.set(base_virtual_tool->to_raw());

        // Add all spool joined items
        {
            VirtualToolIndex tool_1 = *base_virtual_tool;
            while (auto tool_2 = spool_join.get_spool_2(tool_1)) {
                items_.push_back(MappingDisplayItem {
                    .gcode_tool = std::nullopt,
                    .virtual_tool = tool_2,
                    .connection = MappingDisplayItem::Connection::spool_join,
                });
                processed_virtual_tools.set(tool_2->to_raw());

                tool_1 = *tool_2;
            }
        }
    }

    // Now dump all remaining gcode and virtual tools
    {
        auto gcode_tool_it = GcodeToolIndex::all();
        auto virtual_tool_it = VirtualToolIndex::all();
        while (true) {
            MappingDisplayItem item_to_add;

            auto find_next_unused_tool = [](auto &iterator, auto &processed_tools, auto &result) {
                while (!iterator.at_end()) {
                    const auto tool = *iterator;
                    if (processed_tools.test(tool.to_raw())) {
                        // Disabled or already in the list - skip
                        iterator++;
                        continue;
                    }

                    result = tool;
                    processed_tools.set(tool.to_raw());
                    iterator++;
                    break;
                }
            };
            find_next_unused_tool(gcode_tool_it, processed_gcode_tools, item_to_add.gcode_tool);
            find_next_unused_tool(virtual_tool_it, processed_virtual_tools, item_to_add.virtual_tool);

            if (!item_to_add.gcode_tool.has_value() && !item_to_add.virtual_tool.has_value()) {
                // We've displayed all tools
                break;
            }

            items_.push_back(item_to_add);
        }
    }

    // Update tool positions
    {
        gcode_tool_positions_.fill(invalid_tool_position);
        virtual_tool_positions_.fill(invalid_tool_position);
        for (int i = 0; i < (int)items_.size(); i++) {
            const auto &item = items_[i];
            if (item.gcode_tool.has_value()) {
                gcode_tool_positions_[*item.gcode_tool] = i;
            }
            if (item.virtual_tool.has_value()) {
                virtual_tool_positions_[*item.virtual_tool] = i;
            }
        }
    }

    // Re-build the menus
    gcode_tools_menu_.setup_items();
    virtual_tools_menu_.setup_items();

    // Restore item focus to the right item
    focus_tool(original_focused_tool);

    // Update compatibility report
    {
        compat_report_.generate_toolmapping_only({});

        // Status text shows the worst incompatibility - update it
        update_status_text(ShowGuide::no);

        // Routing board shows warning icons, so we need to repaint it
        routing_board_.Invalidate();
    }
}

void FrameToolMapping::update_status_text(ShowGuide show_guide) {
    if (show_guide == ShowGuide::yes) {
        const char *text;
        if (std::holds_alternative<GcodeToolIndex>(selected_tool_for_connecting_)) {
#if HAS_MMU2()
            text = N_("Select MMU filament to connect/disconnect on the right.");
#else
            text = N_("Select printer tool to connect/disconnect on the right.");
#endif
        } else {
            text = N_("Select G-Code filament on the left.");
        }

        bottom_status_.SetText(_(text));
        bottom_status_.SetTextColor(COLOR_WHITE);
        return;
    }

    if (auto failed_check = compat_report_.highest_severity_failed_check(); failed_check) {
        const auto compatibility = failed_check->meta->evaluate_compatibility();
        if (compatibility != buddy::compatibility_checks::CompatibilityLevel::fully_compatible) {
            StringBuilder sb(status_text_);
            match(
                failed_check->tool, //
                [](NoTool) {}, //
                [&](auto tool) {
                    StringViewUtf8Parameters<4> params;
                    sb.append_string_view(tool.compact_display_name(params)); //
                });

            sb.append_string(": ");
            sb.append_string_view(_(failed_check->meta->title));

            // Force update even though the reader has the same ref
            bottom_status_.SetText({});
            bottom_status_.SetText(string_view_utf8::MakeRAM(sb.str()));
            bottom_status_.SetTextColor(buddy::compatibility_checks::compatibility_level_menu_item_color_schemes[compatibility]->text.unfocused);
            return;
        }
    }

    bottom_status_.SetText(_("Ready to print"));
    bottom_status_.SetTextColor(COLOR_GREEN);
}

void FrameToolMapping::update_scroll() {
    const int scroll_offset = virtual_tools_menu_.IsFocused() ? virtual_tools_menu_.scroll_offset() : gcode_tools_menu_.scroll_offset();
    virtual_tools_menu_.set_scroll_offset(scroll_offset);
    gcode_tools_menu_.set_scroll_offset(scroll_offset);

    // Update the scrollbar with everything else, don't wait for the loop
    scrollbar_.WindowEvent(nullptr, GUI_event_t::LOOP, nullptr);

    // Routing board needs to be redrawn when scroll changes
    routing_board_.Invalidate();
}

void FrameToolMapping::windowEvent(window_t *sender, GUI_event_t event, void *const param) {
    if (mapping_update_scheduled_) {
        mapping_update_scheduled_ = false;
        update_mapping();
    }

    switch (event) {

    case GUI_event_t::CHILD_CLICK: {
        const auto response = event_conversion_union { .pvoid = param }.response;
        switch (response) {

        case Response::Print:
            if (!compat_report_.gui_confirm_all_incompatibilities(Response::Cancel, buddy::compatibility_checks::CompatibilityLevel::fully_compatible, buddy::gcode_compatibility::CompatibilityReport::AggregateTools::yes)) {
                return;
            }

            // Don't redraw tools mapping screen since we're leaving
            Screens::Access()->Get()->Validate();

            marlin_client::FSM_response(PhasesPrintPreview::tools_mapping, response);
            return;

        case Response::Abort:
            if (MsgBoxQuestion(_("Do you really want to abort print?"), { Response::Abort, Response::No }) != Response::Abort) {
                return;
            }

            // Don't redraw tools mapping screen since we're leaving
            Screens::Access()->Get()->Validate();

            marlin_client::FSM_response(PhasesPrintPreview::tools_mapping, response);
            return;

        case Response::Remap:
            // Just a helper button to help people realize that they can change the mapping on this screen
            // We've seen some github/social media posts where people thought the tool mapping is just for display and you cannot change it
            focus_first_unconnected_tool(ToolType::gcode_tool);
            update_status_text(ShowGuide::yes);
            return;

        case Response::Filament: {
            // The ChangeAll screen will pop out, this screen will get re-initialized afterwards
            Screens::Access()->Open<ScreenChangeAllFilaments, ScreenChangeAllFilaments::SetupForPrint {}>();
            return;
        }

        default:
            bsod_unreachable();
        }
        break;
    }

    default:
        break;
    }

    window_frame_t::windowEvent(sender, event, param);
}

} // namespace screen_tool_mapping
