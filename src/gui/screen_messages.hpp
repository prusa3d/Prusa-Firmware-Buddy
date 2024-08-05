// screen_messages.hpp
#pragma once
#include <common/circular_buffer.hpp>
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_term.hpp"
#include "screen.hpp"

class MessageBuffer {
    using CharPtr = char *;
    using UnderlyingBuffer = CircularBuffer<CharPtr, 8>;
    UnderlyingBuffer buffer;

public:
    // try getting some message, caller is responsible for freeing it
    [[nodiscard]] bool try_get(CharPtr &txt) {
        if (buffer.is_empty()) {
            return false;
        }
        txt = buffer.get();
        return true;
    }

    // put message into buffer, potentially discarding some old message
    void put(const CharPtr &txt) {
        if (buffer.is_full()) {
            // failure -> make space
            free(buffer.get());
        }
        buffer.put(txt);
        return;
    }
};

struct screen_messages_data_t : public screen_t {
    window_header_t header;
    StatusFooter footer;
    window_term_t term;
    term_buff_t<20, 13> term_buff;

public:
    screen_messages_data_t();

    static MessageBuffer message_buffer;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
