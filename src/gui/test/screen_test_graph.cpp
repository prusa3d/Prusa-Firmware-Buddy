// screen_test_graph.cpp

#include "screen_test_graph.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h"
#include <stdlib.h>

extern void window_temp_scope_add(float temp_ext, float temp_bed);

extern osThreadId displayTaskHandle;

screen_test_graph_t::screen_test_graph_t()
    : window_frame_t()
    , text(this, Rect16(10, 0, 220, 22))
    , button(this, Rect16(10, 220, 100, 22), is_closed_on_click_t::yes)
    , graph(this, Rect16(10, 28, 180, 180))
    , loop_index(0) {
    static const char tst[] = "Test";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));

    static const char rtn[] = "Return";
    button.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)rtn));
}

void screen_test_graph_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_LOOP) {
        if (loop_index == 5) {
            graph.graph_invalid = true;
            //osSignalSet(displayTaskHandle, SIG_DISP_REDRAW);
            gui_invalidate();
            loop_index = 0;
        }
        loop_index++;
    }
}
