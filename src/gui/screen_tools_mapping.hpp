#pragma once

#include <window.hpp>
#include <i18n.h>
#include <window_text.hpp>
#include <client_response.hpp>
#include <window_progress.hpp>
#include <radio_button.hpp>
#include <i_window_menu_item.hpp>
#include <window_line_connector.hpp>
#include <window_icon.hpp>
#include <window_colored_rect.hpp>
#include <gcode_info.hpp>
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#if HAS_TOOLCHANGER() || HAS_MMU2()
    #include <module/prusa/spool_join.hpp>
    #include <module/prusa/tool_mapper.hpp>
#endif
#include <multi_filament_change.hpp>

class ToolsMappingBody : public window_t {
public:
    ToolsMappingBody(window_t *parent, GCodeInfo &gcode_info);

    // 'overloaded' window Hide
    void Hide();
    // 'overloaded' window Show
    void Show();
    // 'overloaded' window Invalidate
    void Invalidate();
    // 'overloaded' window Draw
    void Draw();

    static constexpr auto preview_phase { PhasesPrintPreview::tools_mapping };
    static constexpr const auto &responses_with_print { ClientResponses::GetResponses(preview_phase) };
    static constexpr const auto responses_no_print {
        []() {
            auto modified_resps = responses_with_print;
            for (auto &r : modified_resps) {
                if (r == Response::PRINT || r == Response::Print) {
                    r = Response::_none;
                }
            }
            return modified_resps;
        }()
    }; // precalculated phase responses without PRINT button
    static constexpr auto responses_count { cnt_filled_responses(responses_with_print) };
    static constexpr auto print_response_idx { get_response_idx(responses_with_print, Response::PRINT) };
    static constexpr size_t max_item_rows { 5 };
    static constexpr size_t max_item_text_width { 15 }; // #. FILAM 1.23 // = max 13 chars

    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    /// Used for when srolling up or down, expecting values +- 1 (+1 means go down/right, -1 means go up/left)
    void adjust_index(int difference);
    /// Updates current screen, meant to be called after adjust_index (or from)
    void update_shown_state_after_scroll(uint8_t previous_idx);
    /// Redraws the whole screen to correspond to current state
    void update_shown_state();
    /// Redraws middle connectors based on current state
    void update_middle_connectors();
    /// Redraws icons based on current state
    void update_icons();
    /// Updates bottom_guide based on current state and preprint-check status
    void update_bottom_guide();

    /// Handles when user presses the encoder
    void handle_item_click();
    /// Gets cnt of items in the current state (ie num of physical tools in right state, num of gcode tools otherwise)
    [[nodiscard]] uint8_t get_cnt_current_items();

    const bool drawing_nozzles { true };

    uint8_t current_idx { 0 }; // we have two exclusive columns of items + bottom row of buttons, indexing from top to bottom right.
    uint8_t last_left_idx { 0 }; // when going to the right column, this holds the value of the left index
    uint8_t last_left_real { 0 }; // required for when going back to left column to preselect proper value

    window_text_t left_header;
    window_text_t right_header;
    window_numberless_progress_t left_line;
    window_numberless_progress_t right_line;

    window_line_connector middle_connector;

    /**
     * @brief There's 3 major ways of 'indexing' items:
     * REAL -> The number of tools in Gcode / The dwarf number (given in addr assignment)
     * IDX -> 0 .. Number of left/right items -> ie number of tools mentioned in gcode / number of dwarves found
     * POS -> 0 .. max_item_rows -> Position of the drawn rectangle on the screen (from the top)
     *
     * Theoretically IDX could always be synthesized from POS, but it's much easier to reason about the code if the two arrays are separate (it might be impossible to do it reliable altogether, but readability is enough of an argument to keep it separate)

     * left == gcode (shown on screen as filaments)
     * right == physical (shown on screen as tools)
     */

    std::array<window_text_t, max_item_rows> left_gcode_texts; // "real" gcode tool text windows
    std::array<window_text_t, max_item_rows> right_phys_texts; // "real" physical tool text windows

    // "real" gcode colors (right currently unsupported) -> hacky way of doing colors, no text and coloured background
    std::array<window_colored_rect, max_item_rows> left_gcode_colors;
    std::array<window_icon_t, max_item_rows> left_gcode_icons; // "real" gcode icons for pre-print checks
    std::array<window_icon_t, max_item_rows> right_phys_icons; // "real" physical icons for pre-print checks

    std::array<uint8_t, max_item_rows> left_gcode_idx_to_real; // array mapping left column indices to the real gcode tool
    std::array<uint8_t, max_item_rows> right_phys_idx_to_real; // array mapping right column indices to the real physical tool

    std::array<uint8_t, max_item_rows> left_gcode_pos_to_real; // array mapping left gcodes to their rect position
    std::array<uint8_t, max_item_rows> right_phys_pos_to_real; // array mapping right physicals to their rect position

    std::array<std::array<char, max_item_text_width>, max_item_rows> left_gcode_label_buffers;
    std::array<std::array<char, max_item_text_width>, max_item_rows> right_phys_label_buffers;

    window_text_t bottom_guide; // Currently supports up to two lines of text on our ILI. Shows some text to the user guiding them on what to do/fix
    window_icon_t bottom_icon; // Shows most important icon on the left of the bottom guide

    uint8_t num_unassigned_gcodes { 0 }; // holds the number of currently drawn unassigned gcodes
    uint8_t num_mismatched_filaments { 0 }; // holds the number of physical tools with mismatched filament
    uint8_t num_mismatched_nozzles { 0 }; // holds the number of physical tools with mismatched nozzles
    uint8_t num_unloaded_tools { 0 }; // holds the number of physical tools that are assigned but aren't loaded

    RadioButton bottom_radio;

    enum class State {
        left,
        right,
        done,
    };

    void go_left(); /// Sets current state to left or done depending on if done, handles related tasks
    void go_right(); /// Sets current state to right, handles related tasks
    State state { State::left };
    GCodeInfo &gcode;

    /// returns TRUE if all gcode tools are reasonably mapped to some physical tool
    [[nodiscard]] bool are_all_gcode_tools_mapped() const;

    // When assigning to a right that's already mapped/joined to something, we risk that the previous tool would lose it's mapping.
    void handle_right_steal();

    // ensures all idx_to_real and pos_to_real arrays are in a 'nice to draw' order
    void ensure_nicely_ordered();

    // makes dwarf cheese leds light up, if state right then the hovered physical tool, otherwise all physical tools assigned to hovered filament
    void update_dwarf_lights();

    // refreshes lables within window texts to match current state of loaded filaments
    void refresh_physical_tool_filament_labels();

    // builds default selection config for change all dialog so that physical tools try to load based on what the gcodes want to print with (that they're mapped to)
    MultiFilamentChangeConfig build_changeall_config();

    SpoolJoin joiner;
    ToolMapper mapper;

    bool querying_user { false };
};
