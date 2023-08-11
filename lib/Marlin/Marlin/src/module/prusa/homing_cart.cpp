#include "homing_cart.hpp"

#include <PersistentStorage.h>
#include <config_store/store_instance.hpp>

#include "Marlin.h" // for suspend_auto_report
#include "../motion.h"
#include "../stepper.h"

static const float homing_bump_divisor_dflt[] PROGMEM = HOMING_BUMP_DIVISOR;
static const float homing_bump_divisor_max[] PROGMEM = HOMING_BUMP_DIVISOR_MAX;
static const float homing_bump_divisor_min[] PROGMEM = HOMING_BUMP_DIVISOR_MIN;

/**
 * Turns automatic reports off until destructor is called.
 * Then it sets reports to previous value.
 */
class Temporary_Report_Off {
    bool suspend_reports = false;

public:
    Temporary_Report_Off() {
        suspend_reports = suspend_auto_report;
        suspend_auto_report = true;
    }
    ~Temporary_Report_Off() {
        suspend_auto_report = suspend_reports;
    }
};

/**
 *  Move back and forth to endstop
 * \returns MSCNT position after endstop has been hit
 */
static int32_t home_and_get_mscnt(AxisEnum axis, int axis_home_dir, feedRate_t fr_mm_s, float &probe_offset) {
    probe_offset = homeaxis_single_run(axis, axis_home_dir, fr_mm_s);
    const int32_t mscnt = (axis == X_AXIS) ? stepperX.MSCNT() : stepperY.MSCNT();
    return mscnt;
}

/**
 * \returns calibrated value from EEPROM in microsteps
 *          always 256 microsteps per step, range 0 to 1023
 */
static int get_calibrated_home(const AxisEnum axis, bool &calibrated) {
    uint16_t mscntRead[PersistentStorage::homeSamplesCount];
    calibrated = PersistentStorage::isCalibratedHome(mscntRead, axis);
    return home_modus(mscntRead, PersistentStorage::homeSamplesCount, 96);
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
    bool calibrated;
    const int cal = get_calibrated_home(axis, calibrated);
    if (!calibrated)
        return 0;

    const constexpr float steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;
    switch (axis) {
    case X_AXIS: {
        return ((X_HOME_DIR < 0 ? X_HOME_GAP : -X_HOME_GAP)
            - ((((INVERT_X_DIR) ? -1.f : 1.f) * to_calibrated(cal, stepperX.MSCNT())) / (steps_per_unit[X_AXIS] * (256 / X_MICROSTEPS))));
    }
    case Y_AXIS: {
        return ((Y_HOME_DIR < 0 ? Y_HOME_GAP : -Y_HOME_GAP)
            - ((((INVERT_Y_DIR) ? -1.f : 1.f) * to_calibrated(cal, stepperY.MSCNT())) / (steps_per_unit[Y_AXIS] * (256 / Y_MICROSTEPS))));
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
static int32_t home_and_get_calibration_offset(AxisEnum axis, int axis_home_dir, float &probe_offset, bool store_samples) {
    int32_t calibration_offset = 0;
    bool calibrated = false;
    bool break_loop = false;
    do {
        const int32_t mscnt = home_and_get_mscnt(axis, axis_home_dir, homing_feedrate_mm_s[axis] / homing_bump_divisor[axis], probe_offset);

        if ((probe_offset >= axis_home_min_diff[axis])
            && (probe_offset <= axis_home_max_diff[axis])
            && store_samples) {
            PersistentStorage::pushHomeSample(mscnt, 255, axis); // todo board_temp
        } else {
            break_loop = true;
        }

        calibration_offset = calibrated_offset_mscnt(axis, mscnt, calibrated);
        if (!calibrated)
            calibration_offset = 0;

        SERIAL_ECHO_START();
        SERIAL_ECHOPAIR("Precise homing axis: ", axis);
        SERIAL_ECHOPAIR(" probe_offset: ", probe_offset);
        SERIAL_ECHOPAIR(" HB divisor: ", homing_bump_divisor[axis]);
        SERIAL_ECHOPAIR(" mscnt: ", mscnt);
        SERIAL_ECHOPAIR(" ipos: ", stepper.position_from_startup(axis));
        if (calibrated) {
            SERIAL_ECHOLNPAIR(" Home position diff: ", calibration_offset);
        } else {
            ui.status_printf_P(0, "Calibrating %c axis", axis_codes[axis]);
            SERIAL_ECHOLN(" Not yet calibrated.");
        }

    } while (store_samples && !calibrated && !break_loop);

    return calibration_offset;
}

static void load_divisor_from_eeprom() {
    for (int axis = 0; axis < XY; axis++) {
        const float max = pgm_read_float(&homing_bump_divisor_max[axis]);
        const float min = pgm_read_float(&homing_bump_divisor_min[axis]);
        const float hbd = homing_bump_divisor[axis];
        if (hbd >= min && hbd <= max) {
            continue;
        }

        const float loaded = axis ? config_store().homing_bump_divisor_y.get() : config_store().homing_bump_divisor_x.get();
        if (loaded >= min && loaded <= max) {
            homing_bump_divisor[axis] = loaded;
        } else {
            homing_bump_divisor[axis] = pgm_read_float(&homing_bump_divisor_dflt[axis]);
        }
    }
}

static void homing_failed_update_divisor(AxisEnum axis) {
    homing_bump_divisor[axis] *= HOMING_BUMP_DIVISOR_STEP;
    const float max = pgm_read_float(&homing_bump_divisor_max[axis]);
    const float min = pgm_read_float(&homing_bump_divisor_min[axis]);
    const float hbd = homing_bump_divisor[axis];
    __attribute__((unused)) const float dflt = pgm_read_float(&homing_bump_divisor_dflt[axis]);
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
float home_axis_precise(AxisEnum axis, int axis_home_dir, bool can_calibrate) {
    const int tries = can_calibrate ? PRECISE_HOMING_TRIES : (3 * PRECISE_HOMING_TRIES);
    const int accept_perfect_only_tries = can_calibrate ? (PRECISE_HOMING_TRIES / 3) : 0;
    constexpr int perfect_offset = 96;
    constexpr int acceptable_offset = 288;
    float probe_offset;
    bool first_acceptable = false;

    load_divisor_from_eeprom();

    for (int try_nr = 0; try_nr < tries; ++try_nr) {
        const int32_t calibration_offset = home_and_get_calibration_offset(axis, axis_home_dir, probe_offset, can_calibrate);
        if (planner.draining()) {
            // homing intentionally aborted, do not retry
            break;
        }

        SERIAL_ECHO_START();
        SERIAL_ECHO("Probe classified as ");

        if ((probe_offset < axis_home_min_diff[axis])
            || (probe_offset > axis_home_max_diff[axis])) {
            SERIAL_ECHOLN("failed.");
            ui.status_printf_P(0, "%c axis homing failed, retrying", axis_codes[axis]);
            homing_failed_update_divisor(axis);
        } else if (std::abs(calibration_offset) <= perfect_offset) {
            SERIAL_ECHOLN("perfect.");
            save_divisor_to_eeprom(try_nr, axis);
            return probe_offset;
        } else if ((std::abs(calibration_offset) <= acceptable_offset)) {
            SERIAL_ECHOLN("acceptable.");
            if (try_nr >= accept_perfect_only_tries) {
                save_divisor_to_eeprom(try_nr, axis);
                return probe_offset;
            }
            if (first_acceptable) {
                ui.status_printf_P(0, "Updating precise home point %c axis", axis_codes[axis]);
                homing_failed_update_divisor(axis);
            }
            first_acceptable = true;
        } else {
            SERIAL_ECHOLN("bad.");
            if (can_calibrate) {
                ui.status_printf_P(0, "Updating precise home point %c axis", axis_codes[axis]);
            } else {
                ui.status_printf_P(0, "%c axis homing failed,retrying", axis_codes[axis]);
            }
        }
    }

    SERIAL_ERROR_MSG("Precise homing runs out of tries to get acceptable probe.");
    return probe_offset;
}
