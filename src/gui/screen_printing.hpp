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
#include <option/developer_mode.h>
#include <array>
#include <window_progress.hpp>
#include "screen_printing_end_result.hpp"

enum class printing_state_t : uint8_t {
    INITIAL,
    PRINTING,
    SKIPPABLE_OPERATION,
    PAUSING,
    PAUSED,
    RESUMING,
    ABORTING,
    REHEATING,
    REHEATING_DONE,
    MBL_FAILED,
    STOPPED,
    PRINTED,
    COUNT // setting this state == forced update
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
    bool mmu_maintenance_checked { false }; // Did we check for MMU maintenance

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
     * @brief Shows fields related to time (eg remaining time label + value)
     *
     */
    void show_time_information();
    /**
     * @brief Hides fields related to time (eg remaining time label + value)
     *
     */
    void hide_time_information();

    std::array<char, 5> text_filament; // 999m\0 | 1.2m\0
    std::array<char, FILE_NAME_BUFFER_LEN> text_filename;

    uint32_t message_timer;
    bool stop_pressed;
    bool waiting_for_abort; /// flag specific for stop pressed when MBL is performed
    printing_state_t state__readonly__use_change_print_state;

    float last_e_axis_position;
    const Rect16 popup_rect;

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

    CurrentlyShowing currently_showing { CurrentlyShowing::remaining_time }; // what item is currently shown
    uint32_t last_update_time_s { 0 }; // helper needed to properly rotate
#endif

public:
    screen_printing_data_t();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    void invalidate_print_state();
    void updateTimes();

#if HAS_MINI_DISPLAY()
    void update_print_duration(time_t rawtime);
#endif
    void screen_printing_reprint();
    void set_pause_icon_and_label();
    void set_tune_icon_and_label();
    void set_stop_icon_and_label();
    void change_print_state();

    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

public:
    printing_state_t GetState() const;
    virtual Rect16 GetPopUpRect() override { return popup_rect; }
};
