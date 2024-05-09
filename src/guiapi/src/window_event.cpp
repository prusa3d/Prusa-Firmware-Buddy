// window_event.cpp

#include "window_event.hpp"
#include "log.h"
#include "gui_time.hpp"

LOG_COMPONENT_REF(GUI);

GUI_event_t last_gui_input_event = GUI_event_t::_count;
