
#pragma once
#include "marlin_server.hpp"
#include "timing.h"
#include "gcode/queue.h"
#include "module/planner.h"

/// Helper class to better support serial printing
class SerialPrinting {
public:
    /// Notifies print host about print being paused
    static void pause();

    /// periodically called when in state "printing"
    static void print_loop();

    /// Notify host about print abort
    static void abort();

    /// Notify host about print resume
    static void resume();

    /// Hook called when command is queued via serial line
    /// Used to detect activity of serial print
    static void serial_command_hook(const char *command);

private:
    /// Check if serial printing had timeouted
    /// ie no command was queued for a while
    static bool has_serial_timeouted();

    /// Timeout [ms], after which serial print will be considered as finished
    static constexpr uint32_t serial_printing_screen_timeout = 5 * 1000;

    /// Last time of activity of serial print
    static uint32_t last_serial_indicator_ms;
};
