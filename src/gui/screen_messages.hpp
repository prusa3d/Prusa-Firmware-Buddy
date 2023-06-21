// screen_messages.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.hpp"
#include "window_term.hpp"
#include "screen.hpp"

struct screen_messages_data_t : public AddSuperWindow<screen_t> {
    window_header_t header;
    StatusFooter footer;
    window_term_t term;
    term_buff_t<20, 13> term_buff;

public:
    screen_messages_data_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
