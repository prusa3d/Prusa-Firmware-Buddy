// screen_test_graph.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_temp_graph.hpp"
#include "screen.hpp"

struct screen_test_graph_t : public AddSuperWindow<screen_t> {
    window_text_t text;
    window_text_t button;
    window_temp_graph_t graph;
    uint8_t loop_index;

public:
    screen_test_graph_t();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
