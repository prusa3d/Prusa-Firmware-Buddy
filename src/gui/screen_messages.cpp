#include <feature/print_status_message/print_status_message_mgr.hpp>
#include <feature/print_status_message/print_status_message_formatter_buddy.hpp>
#include "screen_messages.hpp"
#include "ScreenHandler.hpp"
#include <stdlib.h>
#include "i18n.h"
#include <sound.hpp>
#include <utils/string_builder.hpp>

screen_messages_data_t::screen_messages_data_t()
    : screen_t()
    , header(this)
    , footer(this)
    , term(this, GuiDefaults::RectScreenBody.TopLeft(), &term_buff) { // Rect16(10, 28, 11 * 20, 18 * 16))
    header.SetText(_("MESSAGES"));
    ClrMenuTimeoutClose();
    ClrOnSerialClose();
}

void screen_messages_data_t::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        sound::play(SoundType::button_echo);
        close_screen();
        return;

    case GUI_event_t::LOOP:
        print_status_message().walk_history([this](const PrintStatusMessageManager::Record &msg) {
            if (msg.id <= last_message_id) {
                return true;
            }

            ArrayStringBuilder<256> buf;
            PrintStatusMessageFormatterBuddy::format(buf, msg.message);
            term.Printf("%s\n", buf.str());
            last_message_id = msg.id;
            return true;
        });
        break;

    default:
        break;
    }

    screen_t::windowEvent(sender, event, param);
}
