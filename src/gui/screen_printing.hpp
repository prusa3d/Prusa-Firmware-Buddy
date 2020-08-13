//screen_printing.hpp
#pragma once
#include "gui.hpp"
#include "status_footer.h"
#include "window_header.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_spin.hpp"
#include "window_list.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"
#include "IScreenPrinting.hpp"
#include <array>

enum class printing_state_t : uint8_t {
    INITIAL,
    PRINTING,
    PAUSING,
    PAUSED,
    RESUMING,
    ABORTING,
    REHEATING,
    REHEATING_DONE,
    MBL_FAILED,
    PRINTED,
    COUNT //setting this state == forced update
};

enum class item_id_t {
    settings,
    pause,
    pausing,
    stop,
    resume,
    resuming,
    reheating,
    reprint,
    home,
    count
};

constexpr static const size_t POPUP_MSG_DUR_MS = 5000;
constexpr static const size_t MAX_END_TIMESTAMP_SIZE = 14 + 12 + 5; // "dd.mm.yyyy at hh:mm:ss" + safty measures for 3digit where 2 digits should be
constexpr static const size_t MAX_TIMEDUR_STR_SIZE = 9;

class screen_printing_data_t : public IScreenPrinting {
    static constexpr const char *caption = "PRINTING";

    window_text_t w_filename;
    window_progress_t w_progress;
    window_text_t w_time_label;
    window_text_t w_time_value;
    window_text_t w_etime_label;
    window_text_t w_etime_value;

    uint32_t last_print_duration;
    uint32_t last_time_to_end;

    std::array<char, MAX_TIMEDUR_STR_SIZE> text_time_dur;
    std::array<char, MAX_END_TIMESTAMP_SIZE> text_etime;
    //std::array<char, 15> label_etime;  // "Remaining Time" or "Print will end" // nope, if you have only 2 static const strings, you can swap pointers
    string_view_utf8 label_etime;      // not sure if we really must keep this in memory
    std::array<char, 5> text_filament; // 999m\0 | 1.2m\0

    window_text_t w_message; //Messages from onStatusChanged()
    uint32_t message_timer;
    bool message_flag;
    bool stop_pressed;
    bool waiting_for_abort; /// flag specific for stop pressed when MBL is performed
    printing_state_t state__readonly__use_change_print_state;
    uint8_t last_sd_percent_done;

public:
    screen_printing_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
    void invalidate_print_state();
    void open_popup_message();
    void close_popup_message();
    void disable_tune_button();
    void enable_tune_button();
    void update_progress(uint8_t percent, uint16_t print_speed);
    void update_remaining_time(time_t rawtime, uint16_t print_speed);
    void update_end_timestamp(time_t now_sec, uint16_t print_speed);
    void update_print_duration(time_t rawtime);
    void screen_printing_reprint();
    void set_icon_and_label(item_id_t id_to_set, window_icon_t *p_button, window_text_t *lbl);
    void enable_button(window_icon_t *p_button);
    void disable_button(window_icon_t *p_button);
    void set_pause_icon_and_label();
    void set_tune_icon_and_label();
    void set_stop_icon_and_label();
    void change_print_state();

    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;

public:
    printing_state_t GetState() const;
};
