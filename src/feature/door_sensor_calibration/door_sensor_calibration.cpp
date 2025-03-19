#include <feature/door_sensor_calibration/door_sensor_calibration.hpp>
#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <config_store/store_instance.hpp>
#include <marlin_server.hpp>
#include <printers.h>
#include <buddy/unreachable.hpp>
#include <config_store/store_instance.hpp>
#include <buddy/door_sensor.hpp>
#include <option/has_door_sensor_calibration.h>
#include <option/has_door_sensor.h>

static_assert(HAS_DOOR_SENSOR_CALIBRATION(), "Doesn't support door sensor calibration, but tries to compile door sensor calibration");
static_assert(HAS_DOOR_SENSOR(), "Doesn't support door sensor, but tries to compile door sensor calibration");

using marlin_server::wait_for_response;

class DoorSensorCalibration {
public:
    DoorSensorCalibration() = default;

    void run() {
        do {
            run_current_phase();
        } while (curr_phase != PhaseDoorSensorCalibration::finish);
    }

private:
    void fsm_change(const PhaseDoorSensorCalibration phase) {
        last_phase = curr_phase;
        marlin_server::fsm_change(phase);
        curr_phase = phase;
    }

    void skip_ask() {
        switch (wait_for_response(curr_phase)) {
        case Response::Skip:
            config_store().selftest_result_door_sensor.set(TestResult_Skipped);
            fsm_change(PhaseDoorSensorCalibration::finish);
            break;
        case Response::Calibrate:
            fsm_change(PhaseDoorSensorCalibration::confirm_closed);
            break;
        default:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    void continue_abort_phases(const buddy::DoorSensor::State pass_sensor_state, const PhaseDoorSensorCalibration pass_phase, const PhaseDoorSensorCalibration fail_phase) {
        switch (wait_for_response(curr_phase)) {
        case Response::Abort:
            fsm_change(PhaseDoorSensorCalibration::confirm_abort);
            break;
        case Response::Continue:
            if (buddy::door_sensor().state() == pass_sensor_state) {
                fsm_change(pass_phase);
            } else {
                fsm_change(fail_phase);
            }
            break;
        default:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    void done() {
        switch (wait_for_response(curr_phase)) {
        case Response::Continue:
            config_store().selftest_result_door_sensor.set(TestResult_Passed);
            fsm_change(PhaseDoorSensorCalibration::finish);
            break;
        default:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    void repeat() {
        switch (wait_for_response(curr_phase)) {
        case Response::Ok:
            fsm_change(last_phase);
            break;
        default:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    void confirm_abort() {
        switch (wait_for_response(curr_phase)) {
        case Response::Skip:
            config_store().selftest_result_door_sensor.set(TestResult_Skipped);
            fsm_change(PhaseDoorSensorCalibration::finish);
            break;
        case Response::Back:
            fsm_change(last_phase);
            break;
        default:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    void run_current_phase() {
        switch (curr_phase) {
        case PhaseDoorSensorCalibration::confirm_abort:
            confirm_abort();
            break;
        case PhaseDoorSensorCalibration::repeat:
            repeat();
            break;
        case PhaseDoorSensorCalibration::skip_ask:
            skip_ask();
            break;
        case PhaseDoorSensorCalibration::confirm_closed:
            continue_abort_phases(buddy::DoorSensor::State::door_closed, PhaseDoorSensorCalibration::confirm_open, PhaseDoorSensorCalibration::tighten_screw_half);
            break;
        case PhaseDoorSensorCalibration::tighten_screw_half:
            continue_abort_phases(buddy::DoorSensor::State::door_closed, PhaseDoorSensorCalibration::confirm_open, PhaseDoorSensorCalibration::repeat);
            break;
        case PhaseDoorSensorCalibration::confirm_open:
            continue_abort_phases(buddy::DoorSensor::State::door_open, PhaseDoorSensorCalibration::finger_test, PhaseDoorSensorCalibration::loosen_screw_half);
            break;
        case PhaseDoorSensorCalibration::loosen_screw_half:
            continue_abort_phases(buddy::DoorSensor::State::door_open, PhaseDoorSensorCalibration::finger_test, PhaseDoorSensorCalibration::repeat);
            break;
        case PhaseDoorSensorCalibration::finger_test:
            continue_abort_phases(buddy::DoorSensor::State::door_open, PhaseDoorSensorCalibration::done, PhaseDoorSensorCalibration::tighten_screw_quarter);
            break;
        case PhaseDoorSensorCalibration::tighten_screw_quarter:
            continue_abort_phases(buddy::DoorSensor::State::door_open, PhaseDoorSensorCalibration::done, PhaseDoorSensorCalibration::repeat);
            break;
        case PhaseDoorSensorCalibration::done:
            done();
            break;
        case PhaseDoorSensorCalibration::finish:
            BUDDY_UNREACHABLE();
            break;
        }
    }

    PhaseDoorSensorCalibration curr_phase = PhaseDoorSensorCalibration::skip_ask;
    PhaseDoorSensorCalibration last_phase = PhaseDoorSensorCalibration::skip_ask;
};

namespace door_sensor_calibration {
void run() {
    DoorSensorCalibration calibration;
    marlin_server::FSM_Holder holder { PhaseDoorSensorCalibration::skip_ask };
    calibration.run();
}
} // namespace door_sensor_calibration
