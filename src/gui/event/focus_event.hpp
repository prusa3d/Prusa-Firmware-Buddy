#pragma once

#include "gui_event.hpp"

namespace gui_event {

/// Received when an element receives focus
struct FocusInEvent {};
static_assert(GuiEventType<FocusInEvent>);

/// Received when an event loses focus
struct FocusOutEvent {};
static_assert(GuiEventType<FocusOutEvent>);

}; // namespace gui_event
