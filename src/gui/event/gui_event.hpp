#pragma once

#include <concepts>

#include <common/primitive_any.hpp>
#include <bsod/bsod.h>

template <typename T>
concept GuiEventType = true;

class GuiEventContext {

public:
    using AnyEvent = PrimitiveAny<16>;

public:
    GuiEventContext(GuiEventType auto &&event)
        : event(AnyEvent::make(event)) {
    }

public:
    const AnyEvent event;

public:
    /// \returns whether the event is accepted
    /// Accepted events means that it should no longer propagate through the GUI tree
    inline bool is_accepted() const {
        return is_accepted_;
    }

    /// Marks the event as accepted.
    /// Accepted events means that it should no longer propagate through the GUI tree
    inline void accept() {
        debug_assert(!is_accepted_);
        is_accepted_ = true;
    }

private:
    bool is_accepted_ = false;
};
