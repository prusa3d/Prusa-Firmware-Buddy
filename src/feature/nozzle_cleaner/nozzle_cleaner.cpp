#include "nozzle_cleaner.hpp"
#include "Marlin/src/gcode/gcode.h"
#include "raii/scope_guard.hpp"
#include <gcode_loader.hpp>
#include <utils/enum_array.hpp>
#include <Marlin/src/Marlin.h>
#include <Marlin/src/module/motion.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/temperature.h>
#include <option/has_indx.h>
#include <feature/print_status_message/print_status_message_guard.hpp>

#if HAS_INDX()
// The config folds the park point into self-contained constants (SanityCheck.h expands it in every TU);
// keep them tracking the wastebin geometry.
static_assert(X_NOZZLE_PARK_POINT == X_WASTEBIN_POINT);
static_assert(Y_NOZZLE_PARK_POINT == Y_WASTEBIN_POINT + 5.f);
#endif

namespace nozzle_cleaner {

constexpr ConstexprString directory { "nozzle_cleaner" };

static constexpr EnumArray<Sequence, GCodeFile, static_cast<int>(Sequence::_cnt)> sequences {
#if HAS_INDX()
    { Sequence::clean, {
                           .filename = "clean",
                           .directory = directory,
                           .default_gcode = "G750 X0.0 Y118.5 F18000\n"
                                            "G750 Y98.5 F18000\n"
                                            "G750 X-0.5 Y118.5 F18000\n"
                                            "G750 X-0.1 Y98.5 F18000\n"
                                            "G750 X-1.5 Y118.5 F18000\n"
                                            "G750 X-2 Y98.5 F18000\n"
                                            "G750 X-2 Y118.5 F18000\n"
                                            "G750 X0 Y96.5 F18000",
                       } },
        { Sequence::quick_clean, {
                                     .filename = "quick_clean",
                                     .directory = directory,
                                     .default_gcode = "G1 F21000\n"
                                                      "G750 Y98.5 X0\n"
                                                      "G750 Y118.5 X-1.0\n"
                                                      "G750 Y98.5 X-2.0",
                                 } },
        { Sequence::deep_clean, {
                                    .filename = "deep_clean",
                                    .directory = directory,
                                    .default_gcode = "G750 Y98.5 F21000\n"
                                                     "G750 Y118.5 F21000\n"
                                                     "G750 Y98.5 F21000\n"
                                                     "G750 Y118.5 F21000\n"
                                                     "G750 Y98.5 F21000\n"
                                                     "G750 Y118.5 F21000\n"
                                                     "G750 Y92.0 F21000\n"
                                                     "G750 Y118.5 F21000\n"
                                                     "G750 Y98.5 X0.15 F21000\n"
                                                     "G750 Y118.5 X-0.35 F21000\n"
                                                     "G750 Y98.5 X-0.85 F21000\n"
                                                     "G750 Y118.5 X-1.35 F21000\n"
                                                     "G750 Y98.5 X-1.85 F21000\n"
                                                     "G750 Y118.5 X-2.35 F21000\n"
                                                     "G750 Y98.5 X-2.85 F21000\n"
                                                     "G750 Y118.5 X-3.35 F21000\n"
                                                     "G750 Y98.5 X-3.85 F21000\n"
                                                     "G750 Y118.5 X-4.35 F21000\n"
                                                     "G750 Y98.5 X-4.85 F21000\n"
                                                     "G750 Y118.5 X-5.35 F21000\n"
                                                     "G750 Y98.5 X-5.85 F21000\n"
                                                     "G750 Y118.5 X-6.35 F21000\n"
                                                     "G750 Y98.5 X-6.85 F21000\n"
                                                     "G750 Y118.5 X-7.35 F21000\n"
                                                     "G750 Y98.5 X-7.85\n"
                                                     "G750 X-9 F21000",
                                } },
        { Sequence::purge_clean, {
                                     .filename = "purge_clean",
                                     .directory = directory,
                                     .default_gcode = "G750 Y86 F21000 A\n" // Eject poop and move back to purge position
                                                      "G750 Y93 F21000 A\n"
                                                      "G750 Y83 F21000 A\n"
                                                      "G750 Y93 F21000 A\n"
                                                      "G750 Y76 F21000 A\n"
                                                      "G750 Y93 F21000 A\n"
                                                      "G750 Y87 F21000\n"
                                                      "M906 P1\n" // Increase E current for purge
                                                      "G750 E25 F4 L\n" // L: G750 adjusts this E feedrate for the loaded filament
                                                      "M400\n" // planner.synchronize()
                                                      "M1705 N\n" // Autoretract sequence
                                                      "M400\n"
                                                      "M906 P0\n" // Restore E current
                                                      "G750 Y98.5 F21000\n"
                                                      "G750 Y91.5 F21000",
                                 } },
        // Like purge_clean, but without the retract at the end.
        { Sequence::power_panic_purge, {
                                           .filename = "power_panic_purge",
                                           .directory = directory,
                                           .default_gcode = "G750 Y86 F21000 A\n" // Eject poop and move back to purge position
                                                            "G750 Y93 F21000 A\n"
                                                            "G750 Y83 F21000 A\n"
                                                            "G750 Y93 F21000 A\n"
                                                            "G750 Y76 F21000 A\n"
                                                            "G750 Y93 F21000 A\n"
                                                            "G750 Y87 F21000\n"
                                                            "M906 P1\n" // Increase E current for purge
                                                            "G750 E25 F4 L\n" // L: G750 adjusts this E feedrate for the loaded filament
                                                            "M400\n" // planner.synchronize()
                                                            "M906 P0\n" // Restore E current
                                                            "G750 X0.0 Y118.5 F18000\n"
                                                            "G750 Y98.5 F18000\n"
                                                            "G750 X-0.5 Y118.5 F18000\n"
                                                            "G750 X-0.1 Y98.5 F18000\n"
                                                            "G750 X-1.5 Y118.5 F18000\n"
                                                            "G750 X-2 Y98.5 F18000\n"
                                                            "G750 X-2 Y118.5 F18000\n"
                                                            "G750 X0 Y96.5 F18000",
                                       } },
        { Sequence::eject_blob, {
                                    .filename = "eject_blob",
                                    .directory = directory,
                                    .default_gcode = "M204 T5000\n"
                                                     "G750 X0 F21000 A\n"
                                                     "G750 Y86 F21000 A\n"
                                                     "G750 Y93 F21000 A\n"
                                                     "G750 Y83 F21000 A\n"
                                                     "G750 Y93 F21000 A\n"
                                                     "G750 Y76 F21000 A\n"
                                                     "G750 Y93 F21000 A\n"
                                                     "G750 Y86.5 F21000 A\n"
                                                     "G750 Y101.5 F21000 A\n"
                                                     "G750 Y79.5 F21000 A\n"
                                                     "G750 Y93.5 F21000 A\n"
                                                     "G750 Y83.5 F21000 A\n"
                                                     "G750 Y87 F21000",
                                } },
        { Sequence::enter_cleaner, {
                                       .filename = "enter_cleaner",
                                       .directory = directory,
                                       .default_gcode = "G750 X-12 F21000 A\n"
                                                        "G750 Y99.5 F21000 A\n"
                                                        "G750 X0 F21000 A",
                                   } },
        { Sequence::exit_cleaner, {
                                      .filename = "exit_cleaner",
                                      .directory = directory,
                                      .default_gcode = "G750 Y99.5 F21000 A\n"
                                                       "G750 X-12 F21000 A",
                                  } },
        { Sequence::enter_cleaner_from_inside, {
                                                   .filename = "enter_cleaner_from_inside",
                                                   .directory = directory,
                                                   .default_gcode = "G750 Y98.5 F21000 A\n"
                                                                    "G750 X0 F21000 A",
                                               } },
#else
    { Sequence::clean, {
                           .filename = "clean",
                           .directory = directory,
                           .default_gcode = "M106 S80\n" // fan on
                                            "G4 S2\n" // Wait for 2 seconds
                                            "G1 X267.4 Y284.75 F3000\n"
                                            "G1 X253.4 Y284.75 F3000\n"
                                            "G1 X267.4 Y284.75 F3000\n"
                                            "G1 X253.4 Y284.75 F3000\n"
                                            "G1 X253.4 Y305.0 F3000\n"
                                            "M106 S0\n" // fan off
                                            "G1 X254 Y285 F5000\n"
                                            "G1 X248 Y299 F5000\n"
                                            "G1 X235 Y285 F5000\n"
                                            "G1 X243 Y304 F5000\n"
                                            "G1 X230 Y291 F5000\n"
                                            "G1 X235 Y306 F5000\n"
                                            "G1 X224 Y296 F5000\n"
                                            "G1 X226 Y306 F3000\n"
                                            "G1 X248 Y288 F3000\n"
                                            "G1 X247 Y284 F3000\n"
                                            "G1 X229 Y306 F3000\n"
                                            "G1 X254 Y285 F5000\n"
                                            "G1 X248 Y299 F5000\n"
                                            "G1 X235 Y285 F5000\n"
                                            "G1 X243 Y304 F5000\n"
                                            "G1 X230 Y291 F5000\n"
                                            "G1 X235 Y306 F5000\n"
                                            "G1 X224 Y296 F5000\n"
                                            "G1 X226 Y306 F3000\n"
                                            "G1 X248 Y288 F3000\n"
                                            "G1 X247 Y284 F3000\n"
                                            "G1 X229 Y306 F3000",
                       } },
        { Sequence::purge_clean, {
                                     .filename = "purge_clean", .directory = directory,
                                     .default_gcode = "M106 S200\n" // fan on
                                                      "G4 S4\n" // Wait for 4 seconds
                                                      "G1 X267.4 Y284.75 F3000\n"
                                                      "G1 X253.4 Y284.75 F3000\n"
                                                      "G1 X267.4 Y284.75 F3000\n"
                                                      "G1 X253.4 Y284.75 F3000\n"
                                                      "G1 X253.4 Y305.0 F3000\n"
                                                      "M106 S0\n", // fan off
                                 } },
#endif
};

#if HAS_INDX()
namespace {
    bool is_inside_bin() {
        // Same area as parking.cpp's "wastebin area"; constants are rough
        // estimates of the bin's outer extent (INDX_TODO).
        return current_position.x > X_WASTEBIN_SAFE_POINT
            && current_position.y > Y_WASTEBIN_SAFE_POINT;
    }

    /// If the nozzle is outside the bin, runs enter_cleaner first.
    /// @return false on draining/error from the auto-enter, true otherwise.
    bool ensure_safe_cleaning() {
        return is_inside_bin() || load_and_execute(Sequence::enter_cleaner);
    }
} // namespace
#endif

static GCodeLoader &nozzle_cleaner_gcode_loader_instance() {
    static GCodeLoader nozzle_cleaner_gcode_loader;
    return nozzle_cleaner_gcode_loader;
}

std::optional<Sequence> parse_sequence(std::string_view name) {
    for (size_t i = 0; i < sequences.size(); i++) {
        if (name == sequences[i].filename) {
            return static_cast<Sequence>(i);
        }
    }
    return std::nullopt;
}

const GCodeFile &get_sequence(Sequence seq) {
    return sequences[seq];
}

void load_sequence(Sequence seq) {
    nozzle_cleaner_gcode_loader_instance().load_gcode(get_sequence(seq));
}

bool load_and_execute(Sequence seq) {
#if HAS_INDX()
    switch (seq) {
    case Sequence::enter_cleaner:
        if (is_inside_bin()) {
            seq = Sequence::enter_cleaner_from_inside;
        }
        break;
    case Sequence::enter_cleaner_from_inside:
        break;
    case Sequence::exit_cleaner:
        if (!is_inside_bin()) {
            return true;
        }
        break;
    default:
        if (!ensure_safe_cleaning()) {
            return false;
        }
        break;
    }
#endif

    PrintStatusMessageGuard status_message;
    status_message.update<PrintStatusMessage::nozzle_cleaner>({});

    while (true) {
        if (planner.draining()) {
            return false;
        }
        if (is_loader_idle()) {
            load_sequence(seq);
            break;
        }
        idle(true);
    }

    while (!execute()) {
        if (planner.draining()) {
            return false;
        }
        idle(true);
    }

    return true;
}

bool is_loader_idle() {
    return nozzle_cleaner_gcode_loader_instance().is_idle();
}

bool is_loader_buffering() {
    return nozzle_cleaner_gcode_loader_instance().is_buffering();
}

bool execute() {
    // If we are idle or buffering there is no point in trying to execute but we dont want to reset if we are buffering so we just return false
    if (is_loader_idle() || is_loader_buffering()) {
        return false;
    }

    // skip the execution if XY is not homed; we could save ourselves the
    // whole gcode loading too, but that'd add extra conditions and edge cases
    if (!(axes_home_level.is_homed(X_AXIS, AxisHomeLevel::imprecise) && axes_home_level.is_homed(Y_AXIS, AxisHomeLevel::imprecise))) {
        reset();
        return true;
    }

    PrintStatusMessageGuard status_message;
    status_message.update<PrintStatusMessage::nozzle_cleaner>({});

    auto loader_result = nozzle_cleaner_gcode_loader_instance().get_result();
    ScopeGuard resetLoader = [&] { // Ensure the loader is always reset (the exception is if we are buffering or not idle, which is handled above)
        reset();
    };

    const auto print_fan_speed = Temperature::print_fan_speed; // Save print fan before executing the cleaner gcode, we allow the cleaner gcode to play with the print fan
    ScopeGuard restoreFan = [&] {
        thermalManager.set_print_fan_speed(print_fan_speed); // Restore print fan speed after
    };

    // this means the gcode was loaded successfully -> ready to execute it
    if (loader_result.has_value()) {
        GcodeSuite::process_subcommands_now(loader_result.value());
        return true;
    } else { // Here we have an error so we finished unsuccessfully and need to reset the loader for the next use
        return false;
    }
}

void reset() {
    nozzle_cleaner_gcode_loader_instance().reset();
}

} // namespace nozzle_cleaner
