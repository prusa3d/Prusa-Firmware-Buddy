// screen_printing.hpp
#pragma once
#include "status_footer.hpp"
#include "window_header.hpp"
#include "window_roll_text.hpp"
#include "window_icon.hpp"
#include "window_term.hpp"
#include "window_print_progress.hpp"
#include "ScreenPrintingModel.hpp"
#include "print_progress.hpp"
#include "print_time_module.hpp"
#include <guiconfig/guiconfig.h>
#include <buddy/filename_defs.hpp>
#include <option/developer_mode.h>
#include <array>
#include <window_progress.hpp>
#include "screen_printing_end_result.hpp"
#include <feature/print_status_message/print_status_message_guard.hpp>

enum class printing_state_t : uint8_t {
    INITIAL,
    PRINTING,
    SKIPPABLE_OPERATION,
    PAUSING,
    PAUSED,
    RESUMING,
    ABORTING,
    REHEATING,
    STOPPED,
    PRINTED,
};

inline constexpr size_t POPUP_MSG_DUR_MS = 5000;

class screen_printing_data_t : public ScreenPrintingModel {
    static constexpr const char *caption = N_("PRINTING ...");

#if HAS_LARGE_DISPLAY()
    PrintProgress print_progress;

    /**
     * @brief Starts showing end result 'state'
     *
     */
    void start_showing_end_result();
    /**
     * @brief Stops showing end result 'state'
     *
     */
    void stop_showing_end_result();

    /**
     * @brief Hides all fields related to end result
     *
     */
    void hide_end_result_fields();

    bool showing_end_result { false }; // whether currently showing end result 'state'
    bool shown_end_result { false }; // whether end result has ever been shown

    window_icon_t arrow_left;

    WindowProgressCircles rotating_circles;
#endif

    window_roll_text_t w_filename;
    WindowPrintProgress w_progress;
    WindowNumbPrintProgress w_progress_txt;
#if HAS_MINI_DISPLAY()
    window_text_t w_time_label;
    window_text_t w_time_value;
#endif
    window_text_t w_etime_label;
    window_text_t w_etime_value;

    /**
     * @brief Sets visibility to fields related to remaining time (eg remaining time label + value)
     *
     */
    void set_remaining_time_visible(bool visible);

#if HAS_MINI_DISPLAY()
    /**
     * @brief Sets visibility to fields related to end time (eg end time label + value)
     *
     */
    void set_print_time_visible(bool visible);
#endif

    std::array<char, 5> text_filament; // 999m\0 | 1.2m\0
    std::array<char, filename_defs::filename_buffer_size> text_filename;

    uint32_t message_timer;
    bool stop_pressed;
    bool waiting_for_abort; /// flag specific for stop pressed when MBL is performed
    printing_state_t state__readonly__use_change_print_state;

    float last_e_axis_position;

#if HAS_MINI_DISPLAY()
    PrintTime print_time;
    PT_t time_end_format;
#else
    EndResultBody::DateBufferT w_etime_value_buffer;
    EndResultBody end_result_body;

    enum class CurrentlyShowing {
        remaining_time, // time until end of print
        end_time, // 'date' of end of print
        time_to_change, // m600 / m601
        time_since_start, // real printing time since start
        _count,
    };

    static constexpr size_t rotation_time_s { 4 }; // time how often there should be a change between what's currently shown
    size_t valid_count { std::to_underlying(CurrentlyShowing::_count) }; // how many fields are currently valid

    CurrentlyShowing currently_showing { CurrentlyShowing::remaining_time }; // what item is currently shown
    uint32_t last_update_time_s { 0 }; // helper needed to properly rotate

    EnumArray<CurrentlyShowing, std::pair<bool, size_t>, CurrentlyShowing::_count> currently_showing_valid {
        {
            { CurrentlyShowing::remaining_time, { true, 0 } },
            { CurrentlyShowing::end_time, { true, 1 } },
            { CurrentlyShowing::time_to_change, { true, 2 } },
            { CurrentlyShowing::time_since_start, { true, 3 } },
        }
    };
#endif

    window_text_t message_popup;
    PrintStatusMessage current_message;
    std::array<char, 256> message_text;

public:
    screen_printing_data_t();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    void updateTimes();

#if HAS_MINI_DISPLAY()
    void update_print_duration(time_t rawtime);
#endif
    void screen_printing_reprint();
    void set_pause_icon_and_label();
    void set_tune_icon_and_label();
    void set_stop_icon_and_label();
    void change_print_state();
#if HAS_LARGE_DISPLAY()
    /**
     * @brief Updates the validity of the time fields
     *
     * @return true if any of the fields changed validity
     */
    bool update_validities();
    /**
     * @brief Reindexes the rotating circles
     *
     * @return number of valid fields
     */
    size_t reindex_rotating_circles();
#endif
    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

public:
    printing_state_t GetState() const;
};
