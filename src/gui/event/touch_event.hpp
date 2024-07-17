#pragma once

#include "gui_event.hpp"
#include <guitypes.hpp>
#include <option/has_touch.h>

#if HAS_TOUCH()

namespace gui_event {

/// Received when an element receives focus
struct TouchEvent {
    /// Touch point relative to receiving item
    point_ui16_t relative_touch_point;
};
static_assert(GuiEventType<TouchEvent>);

}; // namespace gui_event

#endif
