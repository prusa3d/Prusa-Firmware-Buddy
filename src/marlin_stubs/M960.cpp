#include <marlin_stubs/PrusaGcodeSuite.hpp>

#include <feature/belt_tuning/belt_tuning.hpp>
#include <gcode/gcode_parser.hpp>

/**
 * Belt tuning. Measures belt resonant frequency to determine their tensioning force.
 *
 * See MeasureBeltTensionParams for explanation of the parameters
 *
 * ## Parameters
 * - `B` - Belt system (CoreXY -> only 0)
 * - `A` - Excitation amplitude (in meters)
 *
 * - `F` - Scan start frequency (Hz)
 * - `G` - Scan end frequency (Hz)
 * - `H` - Frequency step (Hz)
 *
 * - `I` - Number of excitation cycles
 * - `J` - Number of wait cycles
 * - `K` - Number of measurement cycles
 *
 * - `M` - Measured harmonic
 *
 * - `C` - Calibrate accelerometer
 * - `S` - Skip tuning (only do setup)
 * - `X` - Skip setup (only do tuning)
 */
void PrusaGcodeSuite::M960() {
    GCodeParser2 parser(GCodeParser2::from_marlin_parser);

    MeasureBeltTensionParams params;
    parser.store_option('B', params.belt_system);
    parser.store_option('A', params.excitation_amplitude_m);

    parser.store_option('F', params.start_frequency_hz);
    parser.store_option('G', params.end_frequency_hz);
    parser.store_option('H', params.frequency_step_hz);

    parser.store_option('I', params.excitation_cycles);
    parser.store_option('J', params.wait_cycles);
    parser.store_option('K', params.measurement_cycles);

    parser.store_option('M', params.measured_harmonic);

    parser.store_option('C', params.calibrate_accelerometer);
    parser.store_option('S', params.skip_tuning);
    parser.store_option('X', params.skip_setup);

    measure_belt_tension(params);
}
