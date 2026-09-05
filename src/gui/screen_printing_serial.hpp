// screen_printing_serial.hpp
#pragma once
#include "ScreenPrintingModel.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_text.hpp"
#include "window_print_progress.hpp"
#include <feature/print_status_message/print_status_message.hpp>
#include <array>

class screen_printing_serial_data_t : public ScreenPrintingModel {
    static constexpr const char *caption = N_("SERIAL PRINTING");

    // Progress bar
    WindowPrintProgress w_progress;

    // Row 1: Progress % (centered)
    window_text_t w_progress_value;

    // Row 2: Elapsed (left) | Remaining (right)
    window_text_t w_elapsed_label;
    window_text_t w_elapsed_value;
    window_text_t w_remaining_label;
    window_text_t w_remaining_value;

    // Buffers
    std::array<char, 32> elapsed_buffer;
    std::array<char, 8> progress_buffer;
    std::array<char, 32> remaining_buffer;

    // Status message display
    window_text_t w_message;
    std::array<char, 64> message_text;
    PrintStatusMessageRecord current_message;

    int last_tick;
    enum class connection_state_t { connected,
        disconnect,
        disconnect_ask,
        disconnecting,
        disconnected };
    connection_state_t connection;

    marlin_server::State last_state;

    void updateTimes();
    void updateStatusMessage();

public:
    screen_printing_serial_data_t();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    virtual void stopAction() override;
    virtual void pauseAction() override;
    virtual void tuneAction() override;
};
