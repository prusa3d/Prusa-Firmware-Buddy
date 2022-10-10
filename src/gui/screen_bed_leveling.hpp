#pragma once

#include "gui.hpp"
#include "status_footer.hpp"
#include "window_text.hpp"
#include "window_icon.hpp"
#include "window_term.hpp"
#include "window_progress.hpp"
#include "screen.hpp"
#include "ScreenHandler.hpp"

class ScreenBedLeveling : public AddSuperWindow<screen_t> {
    enum class state_t {
        idle = 0,
        homing,
        leveling,
        done,
        error
    };

    state_t state { state_t::idle };
    Message<MSG_MAX_LENGTH> m_statusBuffer;
    window_text_t m_statusMsg;
    window_numberless_progress_t m_progressBar;
    window_header_t header;
    StatusFooter footer;
    int m_currentLevelingPointNum { 0 };
    static constexpr uint8_t m_levelingPointCount { 16 };
    uint8_t m_errorSleepCycles { 50 };
    void updateStatusMsg();

public:
    ScreenBedLeveling();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
