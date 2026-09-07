#include <mapi/motion.hpp>
#include <feature/quick_stop/quick_stop.hpp>
#include <marlin_server.hpp>
#include <gcode/gcode.h>
#include <bsod/bsod.h>

void gcode_exception_example() {

    {
        GCodeExceptionHandler outer {
            GCEHandlerExtent::any_move,
            [&] {
                // Handler that gets called if an exception is thrown at this handler
                // We probably need to rehome here because the quick stop probably caused a few skipped steps
            },
        };

        // This move gets executed
        planner.buffer_segment({ 10, 0, 0, 0 });

        {
            GCodeExceptionHandler inner {
                GCEHandlerExtent::any_move,
                [&] {
                    // Handler that gets called if an exception is thrown at this handler
                    // In this example, this does NOT get called
                    debug_assert(false);
                },
            };

            // This move also gets executed
            planner.buffer_segment({ 20, 0, 0, 0 });

            // This will "throw" an exception that will get "caught" by the outer handler
            gcode_exceptions().throw_at(outer);

            // This will not change anything. We're already unwinding to an outer scope
            gcode_exceptions().throw_at(inner);

            // We don't have actual exceptions, so we have to continue executing the code,
            // but the exception mechanism is now in the "unwinding" mode.
            debug_assert(gcode_exceptions().is_unwinding());

            // All moves will be discarded while we're unwinding, so this will not do anything
            planner.buffer_segment({ 30, 0, 0, 0 });

            // Exiting the inner handler scope here.
            // The inner handler callback will NOT get called here, because we are still unwinding till the outer handler scope ends
        }

        // Still unwinding here, this move will also be discarded
        planner.buffer_segment({ 40, 0, 0, 0 });

        // Exiting the outer handler scope here.
        // The unwinding will stop and the outer handler callback WILL be called
    }

    // Not unwinding anymore, the move will be executed
    planner.buffer_segment({ 50, 0, 0, 0 });
}

void gcode_exception_example_2() {
    {
        // Example usage of gcode exceptions region: We want to do a long purge that the user can interrupt at any moment
        GCodeExceptionHandler gceh {
            GCEHandlerExtent::any_move,
            [&] {
                // Callback that gets called if an exception was thrown specifically at this region

                // We've likely lost homing -> rehome
                GcodeSuite::G28_no_parser(true, true, true);
            },
        };

        // Subsrciber that periodically checks whether user aborted the operation.
        // This is done in idle() calls
        Subscriber idle_sub {
            marlin_server::idle_publisher,
            [&] {
                const bool user_pressed_abort = random();
                if (user_pressed_abort) {
                    gceh.trigger();
                }
            },
        };

        // Some long blocking operation that we want to be able to interrupt
        mapi::extruder_move(10000, 0.001);
        planner.synchronize();
    }

    // Quick stop is not triggered outside of the qsr scope anymore (unless an outer GCodeExceptionTryCatch or global quickstop has been triggered)
}
