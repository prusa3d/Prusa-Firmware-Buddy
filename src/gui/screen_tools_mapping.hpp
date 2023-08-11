#pragma once

#include <window.hpp>
#include <i18n.h>
#include <window_text.hpp>
#include <client_response.hpp>
#include <window_progress.hpp>
#include <radio_button.hpp>
#include <i_window_menu_item.hpp>
#include <window_line_connector.hpp>
#include <gcode_info.hpp>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/spool_join.hpp>
    #include <module/prusa/tool_mapper.hpp>
#endif

class ToolsMappingBody : public AddSuperWindow<window_t> {
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
    static constexpr const auto &responses { ClientResponses::GetResponses(preview_phase) };
    static constexpr auto responses_count { cnt_filled_responses(responses) };
    static constexpr auto print_response_idx { get_response_idx(responses, Response::PRINT) };
    static constexpr size_t max_item_rows { 5 };
    static constexpr size_t max_item_text_width { 25 };

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    /// Used for when srolling up or down, expecting values +- 1 (+1 means go down/right, -1 means go up/left)
    void adjust_index(int difference);
    /// Updates current screen, meant to be called after adjust_index (or from)
    void update_drawn_state_after_scroll(uint8_t previous_idx);
    /// Redraws middle connectors based on current state
    void update_middle_connectors();

    /// Handles when user presses the encoder
    void handle_item_click();
    /// Gets cnt of items in the current state (ie num of tools in right state, num of filaments otherwise)
    [[nodiscard]] uint8_t get_cnt_current_items();

    uint8_t current_idx { 0 };   // we have two exclusive columns of items + bottom row of buttons, indexing from top to bottom right.
    uint8_t last_left_idx { 0 }; // when going to the right column, this holds the value of the left index

    window_text_t left_header;
    window_text_t right_header;
    window_numberless_progress_t left_line;
    window_numberless_progress_t right_line;

    window_line_connector middle_connector;

    std::array<window_text_t, max_item_rows> left_filaments;
    std::array<window_text_t, max_item_rows> right_tools;

    static constexpr uint8_t unused_value { std::numeric_limits<uint8_t>::max() };

    /**
     * @brief There's 3 major ways of 'indexing' items:
     * REAL -> The number of filament in Gcode / The number of dwarf (given in addr assignment)
     * IDX -> 0 .. Number of left/right items -> ie number of filaments mentioned / number of dwarves found
     * POS -> 0 .. max_item_rows -> Position of the drawn rectangle on the screen (from the top)
     *
     * Theoretically IDX could always be synthesized from POS, but it's much easier to understand if the two arrays are separate (it might be impossible to do it reliable altogether, but readability is enough of an argument to keep it separate)

     *
     * Sometimes there's also the terms 'logical' and 'physical' thrown around, that's because mapper always mentions what here is 'filaments' as 'physical' and what here is 'tools' is labeled 'logical'. Similarly for left/right in correspondance to the position on the screen, tldr:
     * left == logical == filaments
     * right == physical == tools
     */

    std::array<uint8_t, max_item_rows> left_log_idx_to_real;   // array mapping left column indices to the real logical filament
    std::array<uint8_t, max_item_rows> right_phys_idx_to_real; // array mapping right column indices to the real physical tool

    std::array<uint8_t, max_item_rows> left_log_pos_to_real;   // array mapping left filaments to their rect position
    std::array<uint8_t, max_item_rows> right_phys_pos_to_real; // array mapping right tools to their rect position

    std::array<std::array<char, max_item_text_width>, max_item_rows> left_filament_label_buffers;
    std::array<std::array<char, max_item_text_width>, max_item_rows> right_tool_label_buffers;

    /// Gets underlying window_item with the IDX from current state
    [[nodiscard]] window_text_t &get_real_item(size_t idx);
    /// Gets underlying filament window_item with IDX
    [[nodiscard]] window_text_t &get_real_left_filament(size_t idx);
    /// Gets underlying tool window_item with IDX
    [[nodiscard]] window_text_t &get_real_right_tool(size_t idx);

    window_text_t bottom_guide; // Currently supports up to two lines of text on our ILI

    RadioButton bottom_radio;

    enum class State {
        left,
        right,
        done,
    };

    /// Sets current state to new_state, handles related tasks
    void set_state(State new_state);
    State state { State::left };
    GCodeInfo &gcode;

    /// returns TRUE if all filaments are reasonably mapped to some tool
    [[nodiscard]] bool are_all_filaments_mapped() const;

    // When assigning to a right that's already mapped/joined to something, we risk that the previous tool would lose it's mapping. This lambda handles potential right replacements
    void handle_right_replacement();

    // ensures left_log_indices and right_phys_indices are in a 'nice to print' order
    void ensure_nicely_ordered();

    SpoolJoin joiner;
    ToolMapper mapper;
};
