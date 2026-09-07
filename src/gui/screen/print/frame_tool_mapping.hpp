/// @file
#pragma once

#include <inplace_vector.hpp>

#include <window_frame.hpp>
#include <i_window_menu_item.hpp>
#include <window_menu_virtual.hpp>
#include <window_text.hpp>
#include <radio_button.hpp>
#include <window_colored_rect.hpp>
#include <feature/compatibility_checks/gcode_compatibility.hpp>
#include <window_menu_bar.hpp>

namespace screen_tool_mapping {

/// right position of the right menu rect
constexpr uint16_t right_menu_r = GuiDefaults::ScreenWidth - GuiDefaults::MenuScrollbarWidth;

/// Width of the routing board between the two menus
constexpr uint16_t routing_board_w = 48;

/// Width of the both menus
constexpr uint16_t menu_w = (right_menu_r - routing_board_w) / 2;

constexpr uint16_t menu_item_h = 32;

constexpr uint16_t title_t = GuiDefaults::HeaderHeight;
constexpr uint16_t title_h = 24;

constexpr uint16_t title_line_h = 2;

constexpr auto button_rect = GuiDefaults::GetButtonRect(GuiDefaults::RectScreenNoHeader);
constexpr uint16_t bottom_status_h = 48;

constexpr uint16_t right_menu_l = right_menu_r - menu_w;

constexpr uint16_t menu_t = title_t + title_h + 4;
constexpr uint16_t menu_b = button_rect.Top() - bottom_status_h;
constexpr uint16_t menu_h = menu_b - menu_t;

constexpr auto menu_font = Font::normal;

// A display hack so that we don't have to virtualize the whole IWindowMenuItem::Print
// The whole item is considered an extension
constexpr int16_t menu_item_extension_w = menu_w - GuiDefaults::MenuItemCornerRadius * 2;

constexpr size_t menu_buffer_size = (menu_h + menu_item_h - 1) / menu_item_h;

class FrameToolMapping;

using ToolVariant = std::variant<NoTool, GcodeToolIndex, VirtualToolIndex>;

enum class ToolType : uint8_t {
    gcode_tool,
    virtual_tool
};

enum class ShowGuide : uint8_t {
    no,
    yes
};

struct Config {
    /// We only show nozzle diameters when they are not the same everywhere
    bool show_nozzle_diameter : 1 = false;
};

/// Used by both left (g-code tools) and right (virtual tools) column, represents a single item from frame_.items
class ColumnItem final : public IWindowMenuItem {

public:
    ColumnItem(FrameToolMapping &frame, const ToolType type, const uint8_t item_index);

public:
    /// @returns tool the item represents
    ToolVariant tool() const;

    /// Updates the color scheme and potentially data
    void update();

protected:
    void click_common(IWindowMenu &menu, bool from_touch);

    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;
    void click(IWindowMenu &) override;

private:
    FrameToolMapping &frame_;
    const ToolType type_;
    const uint8_t item_index_;
};

/// Used by both left (g-code tools) and right (virtual tools) column, shows items from frame_.items
class ColumnMenu final : public WindowMenuVirtualSized<menu_buffer_size, 48> {
    friend class FrameToolMapping;

public:
    ColumnMenu(FrameToolMapping &frame, const Rect16 &rect, ToolType type);

    int item_count() const override;

    bool is_item_focusable(int index) const override;

protected:
    void setup_item(ItemVariant &variant, int index) override;

    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    FrameToolMapping &frame_;
    const ToolType type_;
};

/// Item - synchronized for both menus and the routing board
/// Note: Cannot be put directly inside FrameToolMapping because of a GCC bug
struct MappingDisplayItem {
    std::optional<GcodeToolIndex> gcode_tool = std::nullopt;
    std::optional<VirtualToolIndex> virtual_tool = std::nullopt;

    enum class Connection : uint8_t {
        /// Items are not connected at all
        none,

        /// virtual_tool is the main/default tool for gcode_tool
        main_tool,

        /// virtual_tool is a spool join for the previous display item
        /// gcode_tool is not connected
        spool_join,
    };

    Connection connection = Connection::none;
};

class WindowRoutingBoard : public window_t {
    static constexpr uint16_t routing_board_l = menu_w;
    static constexpr uint16_t routing_board_r = right_menu_l;
    static constexpr uint16_t failed_check_icon_size = 16;

    static constexpr Color line_color = COLOR_LIGHT_GRAY;

public:
    WindowRoutingBoard(FrameToolMapping &frame);

protected:
    void unconditionalDraw() override;

private:
    FrameToolMapping &frame_;
};

class FrameToolMapping final : public window_frame_t {
    friend class WindowRoutingBoard;

public:
    /// We need to inherit from window_t because of windowEvent
    /// This is to make the ScreenFSM static_assert shut up
    static constexpr std::monostate needs_to_inherit_from_window_t {};

    FrameToolMapping(window_frame_t *parent);
    ~FrameToolMapping();

public:
    ToolVariant focused_tool();
    void focus_tool(ToolVariant tool);

    void focus_first_unconnected_tool(ToolType tool_type);

    static constexpr int invalid_tool_position = -1;
    struct ToolItemPosition {
        ColumnMenu *menu = nullptr;
        int index = invalid_tool_position;
    };

    /// @returns Menu and index for the specified tool
    /// Can return nullptr if the tool is disabled (and thus not displayed) or if NoTool was provided
    ToolItemPosition tool_item_position(ToolVariant tool);

    /// @returns Menu item for the specified tool
    /// Can return nullptr if the tool is not on the screen (due to scroll window), disabled, or if NoTool was provided
    ColumnItem *tool_menu_item(ToolVariant tool);

    auto selected_tool_for_connecting() const {
        return selected_tool_for_connecting_;
    }

    void select_tool_for_connecting(ToolVariant tool);

    const auto &items() const {
        return items_;
    }

    const auto &config() const {
        return config_;
    }

    bool is_item_focusable(ToolType type, uint8_t index) const;

public:
    /// To be called when mapping changes
    void schedule_mapping_update() {
        // We cannot do anything immediately,
        // this is getting called from ColumnItem procedures
        // and the handling results in the MenuItems being destroyed
        mapping_update_scheduled_ = true;
    }

    /// Synchronizes the scroll between the two menus and the scroll bar
    void update_scroll();

    /// Generally not necessary to call, done in update_compat_report
    /// @param show_guide if yes, shows guide what the user should do instead of compat errors
    void update_status_text(ShowGuide show_guide);

private:
    /// Updates printer HW setup and gcode-info based cache
    /// Only needed once during construction
    void update_config();

    /// Updates menu items.
    /// Necessary to call after each remapping
    void update_mapping();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *const param) override;

private:
    /// Worst case: All virtual tools are mapped to a single physical tools, rest is dangling below
    static constexpr auto max_item_count = VirtualToolIndex::count + GcodeToolIndex::count - 1;

    stdext::inplace_vector<MappingDisplayItem, max_item_count> items_;

    /// Stores position of each GCodeToolIndex in the items_ array
    /// invalid_tool_position for when a tool isn't present
    StrongIndexArray<int, GcodeToolIndex::count, GcodeToolIndex, GcodeToolIndex::to_raw_static> gcode_tool_positions_;

    /// Stores position of each VirtualToolIndex in the items_ array
    /// invalid_tool_position for when a tool isn't present
    StrongIndexArray<int, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> virtual_tool_positions_;

    buddy::gcode_compatibility::CompatibilityReport compat_report_;

    std::array<char, 128> status_text_;

    /// Selected tool for routing operation
    ToolVariant selected_tool_for_connecting_ = NoTool {};

    Config config_;

    /// Set to true if mapping changed from user actions
    /// Updates the widgets in the next suitable event
    bool mapping_update_scheduled_ = false;

    StringViewUtf8Parameters<4> gcode_tools_title_params_, virtual_tools_title_params_;

private: // GUI
    window_text_t gcode_tools_title_;
    WindowColoredRect gcode_tools_title_line_;
    ColumnMenu gcode_tools_menu_;

    window_text_t virtual_tools_title_;
    WindowColoredRect virtual_tools_title_line_;
    ColumnMenu virtual_tools_menu_;

    WindowRoutingBoard routing_board_;

    /// Shared scrollbar for both left and right side
    MenuScrollbar scrollbar_;

    /// Status text above the buttons saying when there is a problem
    window_text_t bottom_status_;

    /// Buttons at the bottom of the screen
    RadioButton bottom_buttons_;
};

} // namespace screen_tool_mapping

using FrameToolMapping = screen_tool_mapping::FrameToolMapping;
