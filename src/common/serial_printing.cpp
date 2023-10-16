#include <serial_printing.hpp>

uint32_t SerialPrinting::last_serial_indicator_ms = 0;

void SerialPrinting::print_loop() {
    if (has_serial_timeouted()) {
        marlin_server::serial_print_finalize();
    }
}

void SerialPrinting::abort() {
    marlin_server::enqueue_gcode("M118 A1 action:cancel");
    marlin_server::enqueue_gcode("M118 A1 action:disconnect");
}

void SerialPrinting::resume() {
    last_serial_indicator_ms = ticks_ms();
    marlin_server::enqueue_gcode("M118 A1 action:resume");
}

void SerialPrinting::pause() {
    marlin_server::enqueue_gcode("M118 A1 action:pause");
}

bool SerialPrinting::has_serial_timeouted() {
    auto curr_time { ticks_ms() };
    if (GCodeQueue::has_commands_queued() || planner.processing()) {
        // refresh timer if there are commands still processing, we want to timeout after command finishes, not after when its queued
        last_serial_indicator_ms = curr_time;
        return false;
    }

    if (last_serial_indicator_ms != 0 && last_serial_indicator_ms <= curr_time && curr_time <= last_serial_indicator_ms + serial_printing_screen_timeout) {
        return false;
    } else {
        return true;
    }
}

void remove_N_prefix(const char *&command) {
    if (*command == 'N') {
        ++command;
        while (*command != ' ') {
            ++command;
        }
        ++command;
    }
}
bool print_indicating_gcode(const char *command) {
    return *command == 'G' || strncmp(command, "M109", 4) == 0 || strncmp(command, "M190", 4) == 0;
}

void SerialPrinting::serial_command_hook(const char *command) {
    // if marlin server already printing on USB, queue command but don't enter serial printing state
    if (!marlin_server::printer_idle())
        return;

    remove_N_prefix(command);
    if (print_indicating_gcode(command)) {
        last_serial_indicator_ms = ticks_ms();
        marlin_server::serial_print_start();
    }
}
