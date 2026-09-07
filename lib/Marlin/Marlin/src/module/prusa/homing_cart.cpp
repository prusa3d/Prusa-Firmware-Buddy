#include "homing_cart.hpp"

#include <config_store/store_instance.hpp>

#include "../motion.h"
#include "../stepper.h"
#include "feature/prusa/crash_recovery.hpp"
#include <lcd/ultralcd.h>
#include "configuration.hpp"
#include <feature/print_status_message/print_status_message_mgr.hpp>

#include <marlin_server.hpp>
#include <bsod/bsod.h>

inline constexpr float HOMING_BUMP_DIVISOR_STEP = 1.03f;
inline constexpr float homing_bump_divisor_dflt[] = HOMING_BUMP_DIVISOR;
inline constexpr float homing_bump_divisor_max[] = HOMING_BUMP_DIVISOR_MAX;
inline constexpr float homing_bump_divisor_min[] = HOMING_BUMP_DIVISOR_MIN;

static constexpr uint8_t homeSamplesCount = config_store_ns::CurrentStore::precise_homing_axis_sample_count;

static void pushHomeSample(uint16_t mscnt, uint8_t axis) {
    const auto index = config_store().precise_homing_sample_history_index.get(axis);
    config_store().precise_homing_sample_history.set(axis * homeSamplesCount + index, mscnt);
    config_store().precise_homing_sample_history_index.set(axis, (index + 1) % homeSamplesCount);
}

static bool isCalibratedHome(uint16_t (&mscnt)[homeSamplesCount], uint8_t axis) {
    const auto offset = axis * homeSamplesCount;

    bool is_calibrated = true;
    auto &store_array = config_store().precise_homing_sample_history;

    for (uint_fast8_t i = 0; i < homeSamplesCount; ++i) {
        auto sample = store_array.get(i + offset);

        if (sample == store_array.get_default_val(i + offset)) {
            is_calibrated = false;
            sample = 0;
        }

        mscnt[i] = sample;
    }
    return is_calibrated;
}

[[maybe_unused]] static void erase_axis(uint8_t axis) {
    config_store().precise_homing_sample_history_index.set_to_default(axis);

    for (int i = homeSamplesCount * axis, e = homeSamplesCount * axis + homeSamplesCount; i < e; i++) {
        config_store().precise_homing_sample_history.set_to_default(i);
    }
}

/**
 * \returns calibrated value from EEPROM in microsteps
 *          always 256 microsteps per step, range 0 to 1023
 */
static int get_calibrated_home(const AxisEnum axis, bool &calibrated) {
    uint16_t mscntRead[homeSamplesCount];
    calibrated = isCalibratedHome(mscntRead, axis);
    return home_modus(mscntRead, homeSamplesCount, 96);
}

/**
 * \param axis axis to be evaluated
 * \param mscnt measured motor position if known,
 *              otherwise, current motor position will be taken
 * \returns shortest distance of the motor to the calibrated position in microsteps,
 *          always 256 microsteps per step, range -512 to 512
 */
static int calibrated_offset_mscnt(const AxisEnum axis, const int mscnt, bool &calibrated) {
    const int cal = get_calibrated_home(axis, calibrated);
    return to_calibrated(cal, mscnt);
}

/**
 * \returns offset [mm] to be subtracted from the current axis position to have correct position
 */
float calibrated_home_offset(const AxisEnum axis) {
    if (axis >= config_store_ns::CurrentStore::precise_homing_axis_count) {
        return 0;
    }

    bool calibrated;
    const int cal = get_calibrated_home(axis, calibrated);
    if (!calibrated) {
        return 0;
    }

    switch (axis) {
    case X_AXIS: {
        return ((X_HOME_DIR < 0 ? X_HOME_GAP : -X_HOME_GAP)
            - ((((INVERT_X_DIR) ? -1.f : 1.f) * to_calibrated(cal, stepperX.MSCNT())) / (get_steps_per_unit_x() * (256 / get_microsteps_x()))));
    }
    case Y_AXIS: {
        return ((Y_HOME_DIR < 0 ? Y_HOME_GAP : -Y_HOME_GAP)
            - ((((INVERT_Y_DIR) ? -1.f : 1.f) * to_calibrated(cal, stepperY.MSCNT())) / (get_steps_per_unit_y() * (256 / get_microsteps_y()))));
    }
    default:;
    }
    return 0;
}

/**
 * \brief Home and get offset from calibrated point
 *
 * \returns offset from calibrated point -512 .. +512
 */
static int32_t home_and_get_calibration_offset(AxisEnum axis, int axis_home_dir, float &probe_offset, bool store_samples, float fr_mm_s = 0) {
    int32_t calibration_offset = 0;
    bool calibrated = false;
    bool break_loop = false;
    fr_mm_s = fr_mm_s != 0.0f ? fr_mm_s : homing_feedrate_mm_s[axis];

    PrintStatusMessageGuard status_guard;

    do {
        probe_offset = homeaxis_single_run(axis, axis_home_dir, fr_mm_s / homing_bump_divisor[axis]);
        const int32_t mscnt = (axis == X_AXIS) ? stepperX.MSCNT() : stepperY.MSCNT();

        if ((probe_offset >= axis_home_min_diff(axis))
            && (probe_offset <= axis_home_max_diff(axis))
            && store_samples) {
            pushHomeSample(mscnt, axis);
        } else {
            break_loop = true;
        }

        calibration_offset = calibrated_offset_mscnt(axis, mscnt, calibrated);
        if (!calibrated) {
            calibration_offset = 0;
        }

        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR("    homing probe offset: ", probe_offset);
        SERIAL_ECHOPAIR(" divisor: ", homing_bump_divisor[axis]);
        SERIAL_ECHOPAIR(" mscnt: ", mscnt);
        if (calibrated) {
            SERIAL_ECHOLNPAIR(" calibration offset: ", calibration_offset);
        } else {
            status_guard.update<PrintStatusMessage::calibrating_axis>({ .axis = axis });
            // I _could_ use SERIAL_ECHOLN(), but that somehow cuts off the end
            // of the printed string. Very nice.
            SERIAL_ECHOLNPAIR(" Not yet", " calibrated.");
        }

    } while (store_samples && !calibrated && !break_loop);

    return calibration_offset;
}

static void load_divisor_from_eeprom() {
    LOOP_XY(axis) {
        const float value = axis == X_AXIS ? config_store().homing_bump_divisor_x.get() : config_store().homing_bump_divisor_y.get();
        if (value >= homing_bump_divisor_min[axis] && value <= homing_bump_divisor_max[axis]) {
            homing_bump_divisor[axis] = value;
        } else {
            homing_bump_divisor[axis] = homing_bump_divisor_dflt[axis];
        }
    }
}

static void homing_failed_update_divisor(AxisEnum axis) {
    homing_bump_divisor[axis] *= HOMING_BUMP_DIVISOR_STEP;
    const float max = homing_bump_divisor_max[axis];
    const float min = homing_bump_divisor_min[axis];
    const float hbd = homing_bump_divisor[axis];
    if (hbd > max || /* shouldnt happen, just to make sure */ hbd < min) {
        homing_bump_divisor[axis] = min;
    }
}

static void save_divisor_to_eeprom(int try_nr, AxisEnum axis) {
    if (try_nr > 0 && axis < XY) {
        if (axis) {
            config_store().homing_bump_divisor_y.set(homing_bump_divisor[axis]);
        } else {
            config_store().homing_bump_divisor_x.set(homing_bump_divisor[axis]);
        }
    }
}

#if PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_MK3_5()
inline constexpr uint8_t HOMING_SENSITIVITY_CALIBRATION_TRIES = 4;

static void store_homing_sensitivity(AxisEnum axis, int16_t value) {
    switch (axis) {
    case X_AXIS:
        config_store().homing_sens_x.set(value);
        break;
    case Y_AXIS:
        config_store().homing_sens_y.set(value);
        break;
    default:
        debug_assert(false && "invalid axis index");
    }
}

class SensitivityCalibration {
    struct AvgData {
        float probe_offset_avg { 0.0 };
        uint8_t n { 0 };

        bool is_bad() const {
            // Note: NAN happens if a homing fails
            return isnan(probe_offset_avg) || ((probe_offset_avg * n) > 0.6);
        }

        bool is_valid() const {
            return n > 0 && !is_bad();
        }
    };

    static constexpr int16_t MIDDLE_SENSITIVITY = (XY_STALL_SENSITIVITY_MIN + XY_STALL_SENSITIVITY_MAX) / 2;

    AxisEnum axis;
    std::array<AvgData, XY_STALL_SENSITIVITY_MAX - XY_STALL_SENSITIVITY_MIN + 1> avgs;
    // start in the middle of the sensitivity range
    int16_t current_sensitivity { MIDDLE_SENSITIVITY };
    bool calibrated { false };
    AvgData *best_data { nullptr };

    size_t s2i(int16_t sensitivity) {
        return sensitivity - XY_STALL_SENSITIVITY_MIN;
    }

    int16_t i2s(size_t index) {
        return index + XY_STALL_SENSITIVITY_MIN;
    }

    bool have_good_data() {
        return best_data != nullptr && !best_data->is_bad();
    }

    bool next_sensitivity() {
        auto &data = avgs[s2i(current_sensitivity)];

        if (best_data == nullptr || data.probe_offset_avg < best_data->probe_offset_avg) {
            best_data = &data;
        }

        // go through the upper half of the range first
        // (less chance to crush into the opposing endstop)
        if (current_sensitivity >= MIDDLE_SENSITIVITY) {
            // continue up the upper half as long as the results are not getting worse
            if (current_sensitivity < XY_STALL_SENSITIVITY_MAX && (!have_good_data() || !data.is_bad())) {
                ++current_sensitivity;
                return true;
            }

            // if the middle was not bad or we haven't found a good result,
            // move to the lower half of the range
            auto &middle_data = avgs[s2i(MIDDLE_SENSITIVITY)];
            if (MIDDLE_SENSITIVITY > XY_STALL_SENSITIVITY_MIN && (!have_good_data() || !middle_data.is_bad())) {
                current_sensitivity = MIDDLE_SENSITIVITY - 1;
                return true;
            }
        } else {
            // go down the lower half of the range as long as the results are not getting worse
            if (current_sensitivity > XY_STALL_SENSITIVITY_MIN && (!have_good_data() || !data.is_bad())) {
                --current_sensitivity;
                return true;
            }
        }

        return false;
    }

public:
    SensitivityCalibration(AxisEnum axis, bool force_recalibration = false)
        : axis(axis) {
        SERIAL_ECHO_START();
        SERIAL_ECHO("Homing sensitivity: ");
        if (crash_s.home_sensitivity[axis] != config_store_ns::stallguard_sensitivity_unset) {
            if (force_recalibration) {
                SERIAL_ECHOLNPAIR("forcing recalibration, sensitivity ", current_sensitivity);
                crash_s.home_sensitivity[axis] = current_sensitivity;
            } else {
                SERIAL_ECHOLNPAIR("already calibrated to ", crash_s.home_sensitivity[axis]);
                calibrated = true;
            }
        } else {
            SERIAL_ECHOLNPAIR("starting calibration at sensitivity ", current_sensitivity);
            crash_s.home_sensitivity[axis] = current_sensitivity;
        }
    }

    void update_probe_offset_avg(float probe_offset) {
        auto &data = avgs[s2i(current_sensitivity)];

        data.probe_offset_avg = data.probe_offset_avg + (abs(probe_offset) - data.probe_offset_avg) / ++data.n;

        // If the average offset multiplied by number of tries is over 0.6mm,
        // the sensitivity does not work well and we can move on to the next one
        if (data.is_bad() || data.n >= HOMING_SENSITIVITY_CALIBRATION_TRIES) {
            if (next_sensitivity()) {
                crash_s.home_sensitivity[axis] = current_sensitivity;
                SERIAL_ECHO_START();
                SERIAL_ECHOLNPAIR("Homing sensitivity: calibrating at sensitivity ", current_sensitivity);
            } else {
                auto it_last = std::ranges::find_if(avgs, [](const AvgData &d) { return d.is_valid(); });

                // make it_last point to the last smallest average in the array
                for (auto it = avgs.begin(); it < avgs.end(); ++it) {
                    if (it->is_valid() && it->probe_offset_avg <= it_last->probe_offset_avg) {
                        it_last = it;
                    }
                }

                // make it_first point to the first smallest average in the
                // array (in a continuous range from it_last)
                auto it_first = it_last;
                while (it_first > avgs.begin()) {
                    if ((it_first - 1)->is_valid() && (it_first - 1)->probe_offset_avg == it_last->probe_offset_avg) {
                        --it_first;
                    } else {
                        break;
                    }
                }

                // move it_last forward so that it points behind the last smallest average
                ++it_last;

                // select the sensitivity in the middle of the range, rounding up
                auto it_selected = it_first + (it_last - it_first) / 2;

                current_sensitivity = i2s(it_selected - avgs.begin());
                crash_s.home_sensitivity[axis] = current_sensitivity;

                SERIAL_ECHO_START();
                SERIAL_ECHOPAIR("Homing sensitivity: calibrated to ", current_sensitivity);
                SERIAL_ECHOLNPAIR(" of probe offset avg: ", it_selected->probe_offset_avg);
                store_homing_sensitivity(axis, current_sensitivity);
                calibrated = true;
            }
        }
    }

    bool is_calibrated() { return calibrated; }

    int8_t get_current_sensitivity() { return current_sensitivity; }
};
#endif

/**
 * \brief Home and decide if position of both probes is close enough to calibrated home position
 *
 * Do homing probe and decide if can be accepted. If it is not
 * good enough do the probe again up to PRECISE_HOMING_TRIES times.
 *
 * \param axis axis to be homed (cartesian printers only)
 * \param axis_home_dir direction where the home of the axis is
 * \param can_calibrate Can be calibrated home position updated?
 * \return Distance between two probes in mm.
 */
float home_axis_precise(AxisEnum axis, int axis_home_dir, bool can_calibrate, float fr_mm_s) {
    const int tries = can_calibrate ? PRECISE_HOMING_TRIES : (3 * PRECISE_HOMING_TRIES);
    int accept_perfect_only_tries = can_calibrate ? 3 : 3 * 3;
    constexpr int perfect_offset = 96;
    constexpr int acceptable_offset = 288;
    float probe_offset;
    bool first_acceptable = false;
    [[maybe_unused]] bool done_sens_calib_reset = false;

    load_divisor_from_eeprom();

    PrintStatusMessageGuard status_guard;

    bool success = false;
    int try_nr = 0;
    for (; try_nr < tries; ++try_nr) {
        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR("== Precise Homing axis ", axis_codes[axis]);
        SERIAL_ECHOPAIR(" try ", try_nr);
        SERIAL_ECHOLN(" ==");

        // If homing is failing, try to recalibrate sensitivity. We do this
        // after we couldn't home perfectly, and increase the perfect only
        // tries so that we still try to home perfectly after recalibrating
        // the sensitivity.
        bool reset_sens_calibration = false;
        if (can_calibrate && try_nr >= accept_perfect_only_tries && !done_sens_calib_reset) {
            reset_sens_calibration = true;
            done_sens_calib_reset = true;
            accept_perfect_only_tries *= 2;
        }

        SensitivityCalibration sens_calibration { axis, reset_sens_calibration };

        while (can_calibrate && !sens_calibration.is_calibrated()) {
            status_guard.update<PrintStatusMessage::calibrating_axis>({ .axis = axis });
            home_and_get_calibration_offset(axis, axis_home_dir, probe_offset, false, fr_mm_s);
            if (planner.draining()) {
                // ensure we do not save aborted calibration probes
                break;
            }
            sens_calibration.update_probe_offset_avg(probe_offset);

            if (sens_calibration.is_calibrated()) {
                erase_axis(axis);
            }
        }

        const int32_t calibration_offset = home_and_get_calibration_offset(axis, axis_home_dir, probe_offset, can_calibrate, fr_mm_s);
        if (planner.draining()) {
            // homing intentionally aborted, do not retry
            break;
        }

        SERIAL_ECHO_START();
        SERIAL_ECHO("Probe classified as ");

        if ((probe_offset < axis_home_min_diff(axis))
            || (probe_offset > axis_home_max_diff(axis))) {
            SERIAL_ECHOLN("failed.");
            status_guard.update<PrintStatusMessage::homing_retrying>({ .axis = axis });
            homing_failed_update_divisor(axis);

        } else if (std::abs(calibration_offset) <= perfect_offset) {
            SERIAL_ECHOLN("perfect.");
            success = true;
            break;

        } else if ((std::abs(calibration_offset) <= acceptable_offset)) {
            SERIAL_ECHOLN("acceptable.");
            if (try_nr >= accept_perfect_only_tries) {
                success = true;
                break;
            }

            if (first_acceptable) {
                status_guard.update<PrintStatusMessage::homing_refining>({ .axis = axis });
                homing_failed_update_divisor(axis);
            }
            first_acceptable = true;

        } else {
            SERIAL_ECHOLN("bad.");
            if (can_calibrate) {
                status_guard.update<PrintStatusMessage::homing_refining>({ .axis = axis });
            } else {
                status_guard.update<PrintStatusMessage::homing_retrying>({ .axis = axis });
            }
        }
    }

    if (success) {
        save_divisor_to_eeprom(try_nr, axis);

        // Mark the axis as precisely homed
        axes_home_level[axis] = AxisHomeLevel::full;

    } else {
        SERIAL_ERROR_MSG("Precise homing runs out of tries to get acceptable probe.");
    }

    // What about some failure reporting, hm?
    return probe_offset;
}
