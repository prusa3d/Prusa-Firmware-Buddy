#include <serial_printing.hpp>
#include <state/printer_state.hpp>
#include <option/developer_mode.h>
#include <config_store/store_instance.hpp>

uint32_t SerialPrinting::last_serial_indicator_ms = 0;

void SerialPrinting::print_loop() {
    if (has_serial_timeouted()) {
        marlin_server::serial_print_finalize();
    }
}

void SerialPrinting::abort() {
    marlin_server::enqueue_gcode("M118 A1 action:cancel");
}

void SerialPrinting::resume() {
    last_serial_indicator_ms = ticks_ms();
    GCodeQueue::pause_serial_commands = false;
    marlin_server::enqueue_gcode("M118 A1 action:resume");
}

void SerialPrinting::pause() {
    GCodeQueue::pause_serial_commands = false;
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
            if (*command == '\0') {
                return;
            }
            ++command;
        }
        ++command;
    }
}
bool print_indicating_gcode(const char *command) {
    if (*command == '\0') {
        return false;
    }
    if (*command == 'G') {
        return true;
    } else if (*command == 'M') {
        int num = atoi(command + 1);
        switch (num) {
        case 73:
        case 74:
        case 109:
        case 190:
            return true;
            break;
        default:
            return false;
            break;
        }
    }
    return false;
}

void SerialPrinting::serial_command_hook(const char *command) {
    // never enter serial printing in developer mode as it breaks live debugging
    if (option::developer_mode) {
        return;
    }

    // if marlin server already printing, or is not able to start print, do not enter serial printing state
    // command will be still queued for execution regardless of this.
    if (!printer_state::remote_print_ready(true)) {
        return;
    }

    if (!config_store().serial_print_screen_enabled.get()) {
        return;
    }

    remove_N_prefix(command);
    if (print_indicating_gcode(command)) {
        last_serial_indicator_ms = ticks_ms();
        marlin_server::serial_print_start();
    }
}
