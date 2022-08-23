#include "screen_bed_leveling.hpp"
#include "log.h"
#include "ScreenHandler.hpp"
#include "gui.hpp"

ScreenBedLeveling::ScreenBedLeveling()
    : AddSuperWindow<screen_t>()
    , m_statusMsg(this, { 10, 128, 220, 80 }, is_multiline::no)
    , m_progressBar(this, Rect16(10, 70, GuiDefaults::RectScreen.Width() - 2 * 10, 16))
    , header(this)
    , footer(this) {
    header.SetIcon(IDR_PNG_heatbed_16px);
    header.SetText(_("BED LEVELING"));
    Screens::Access()->DisableMenuTimeout();
    const uint8_t *recastPointer = reinterpret_cast<const uint8_t *>((const char *)this->m_statusBuffer);
    this->m_statusMsg.SetText(string_view_utf8::MakeRAM(recastPointer));
}

void ScreenBedLeveling::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        switch (this->state) {
        case state_t::idle: // Start homing
            // Start by clearing events and homing
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            marlin_event_clr(MARLIN_EVT_CommandBegin);
            marlin_event_clr(MARLIN_EVT_CommandEnd);
            marlin_gcode("G28");
            this->m_statusMsg.SetText(_("Homing..."));
            while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                marlin_client_loop();
            this->state = state_t::homing;
            break;

        case state_t::homing:
            // If homing is finished, start mesh bed leveling
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                marlin_gcode("G29");
                while (!marlin_event_clr(MARLIN_EVT_CommandBegin))
                    marlin_client_loop();
                this->state = state_t::leveling;
                this->updateStatusMsg();
            }
            break;

        case state_t::leveling: {
            this->updateStatusMsg();

            // If leveling is finished, jump the an end state
            if (marlin_event_clr(MARLIN_EVT_CommandEnd)) {
                if (marlin_error(MARLIN_ERR_ProbingFailed) || m_currentLevelingPointNum != m_levelingPointCount) {
                    this->state = state_t::error;
                } else {
                    this->state = state_t::done;
                }
            }
            break;
        }
        case state_t::done:
            Screens::Access()->Close();
            break;
        case state_t::error:
            marlin_error_clr(MARLIN_ERR_ProbingFailed);
            m_statusMsg.SetText(_("Bed leveling failed"));
            if (!m_errorSleepCycles--)
                Screens::Access()->Close();
            break;
        default:
            Screens::Access()->Close();
            break;
        }
    }

    SuperWindowEvent(sender, event, param);
}

void ScreenBedLeveling::updateStatusMsg() {
    // Update status message
    bool gotNewMessage = MsgCircleBuffer().ConsumeLast(this->m_statusBuffer);
    if (gotNewMessage) {
        this->m_progressBar.SetProgressPercent((++m_currentLevelingPointNum / (float)m_levelingPointCount) * 100.0f);
        const uint8_t *recastPointer = reinterpret_cast<const uint8_t *>((const char *)this->m_statusBuffer);
        this->m_statusMsg.SetText(string_view_utf8::MakeRAM(recastPointer));
        this->m_statusMsg.Invalidate();
    }
}
