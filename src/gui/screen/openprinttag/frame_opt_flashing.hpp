/// @file
#pragma once

#include <radio_button.hpp>
#include <window_frame.hpp>
#include <window_numb.hpp>
#include <window_progress.hpp>
#include <window_text.hpp>

namespace buddy::openprinttag {

/// Progress frame for the NFC reader firmware update: title, bar, percentage, text and a radio.
class FrameOPTFlashing : public window_frame_t {

public:
    FrameOPTFlashing(window_frame_t *parent);

    inline auto &radio() {
        return radio_;
    }

    void set_progress(uint8_t percent);
    void set_stalled(bool stalled);
    void set_done();

private:
    window_text_t title_;
    WindowRoundedProgressBar progress_bar_;
    window_numb_t progress_number_;
    window_text_t label_;
    RadioButton radio_;
};

} // namespace buddy::openprinttag
