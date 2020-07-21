//screen_test_graph.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_temp_graph.hpp"

struct screen_test_graph_t : public window_frame_t {
    window_text_t text;
    window_text_t button;
    window_temp_graph_t graph;
    uint8_t loop_index;

public:
    screen_test_graph_t();

private:
    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
