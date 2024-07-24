#pragma once

#include "gui_event.hpp"

namespace gui_event {

/// Received when an element receives focus
struct FocusInEvent {
    inline bool operator==(const FocusInEvent &) const = default;
};
static_assert(GuiEventType<FocusInEvent>);

/// Received when an event loses focus
struct FocusOutEvent {
    inline bool operator==(const FocusOutEvent &) const = default;
};
static_assert(GuiEventType<FocusOutEvent>);

}; // namespace gui_event
