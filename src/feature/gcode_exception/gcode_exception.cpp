#include "gcode_exception.hpp"

#include <algorithm>

#include <module/planner.h>
#include <bsod/bsod.h>
#include <utils/storage/single_linked_list.hpp>
#include <feature/precise_stepping/precise_stepping.hpp>

void GCodeExceptionManager::throw_at(GCodeExceptionHandlerBase *handler) {
    if (handler) {
        handler->is_being_thrown_at_ = true;
    } else {
        is_unwinding_unhandled_exception_ = true;
    }

    if (is_unwinding_) {
        return;
    }

    // Immediately stop motion that has already been planned
    PreciseStepping::quick_stop();

    // Reset the block delay for the first movement
    planner.delay_before_delivering = 0;

    // Start discarding every action till we finish unwinding
    is_unwinding_ = true;
    throw_count_++;
}

bool GCodeExceptionManager::finish_unwinding_unhandled_exception() {
    // Global resume should never happen while there is any handler active
    debug_assert(handlers_.empty());

    if (!is_unwinding_unhandled_exception_) {
        return false;
    }

    is_unwinding_unhandled_exception_ = false;

    // This should never fail
    [[maybe_unused]] const auto resume_result = maybe_finish_unwinding();
    debug_assert(resume_result);

    return true;
}

void GCodeExceptionManager::report_xyz_move() {
    if (!can_move_xyz_) {
        bsod("GCodeExceptionHandler unsupproted XYZ move");
    }
}

bool GCodeExceptionManager::maybe_finish_unwinding() {
    // No exception to be handled -> nothing to resume
    if (!is_unwinding_) {
        return false;
    }

    // Unhandled exceptions need to be processed explicitly by finish_unwinding_unhandled_exception()
    if (is_unwinding_unhandled_exception_) {
        return false;
    }

    for (const auto &handler : handlers_) {
        if (handler.is_being_thrown_at_) {
            // Some active are still triggered - we cannot resume
            return false;
        }
    }

    if (PreciseStepping::stopping()) {
        // If stop_pending hasn't been processed yet, do so now before new moves are processed
        PreciseStepping::loop();
        debug_assert(!PreciseStepping::stopping());
    }

    // Allow planning new moves
    is_unwinding_ = false;

    return true;
}

void GCodeExceptionManager::update_handlers_cache() {
    can_move_xyz_ = std::ranges::all_of(handlers_, [](const GCodeExceptionHandlerBase &handler) -> bool { return handler.extent_ == GCEHandlerExtent::any_move; });
}

GCodeExceptionHandlerBase::GCodeExceptionHandlerBase(GCEHandlerExtent interruptable_movements)
    : extent_(interruptable_movements) {
    auto &ge = gcode_exceptions();

    // Do not allow quickstopping any other moves
    planner.synchronize();

    ge.handlers_.push_front(*this);
    ge.update_handlers_cache();
}

bool GCodeExceptionHandlerBase::finalize() {
    auto &ge = gcode_exceptions();

    planner.synchronize();

    ge.handlers_.remove(*this);
    ge.update_handlers_cache();

    // If this was the only triggered handler, resume_local should resume the execution
    return ge.maybe_finish_unwinding();
}
