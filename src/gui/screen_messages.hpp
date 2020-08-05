//screen_messages.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"

struct screen_messages_data_t : public window_frame_t {
    window_header_t header;
    status_footer_t footer;
    window_list_t list;

public:
    screen_messages_data_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
