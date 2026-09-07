#include "motordriver_util.h"

#include <atomic>
#include <timing.h>

#if HAS_TRINAMIC
    #include <module/stepper/trinamic.h>
    #include <feature/tmc_util.h>
#endif

#if TMC_HAS_SPI
    #include <SPI.h>
#endif

#include <option/has_planner.h>
#include <bsod/bsod.h>
#if HAS_PLANNER()
    #include <module/planner.h>
#endif

#if USE_SENSORLESS
std::bitset<3> axis_crash_detection_enabled {};

bool enable_crash_detection(AxisEnum axis) {
    axis_crash_detection_enabled.set(axis);
    #if HAS_TRINAMIC
    return tmc_enable_stallguard(stepper_axis(axis));
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    bsod("Unknown driver");
    #endif
}

void disable_crash_detection(AxisEnum axis, bool restore_stealth) {
    axis_crash_detection_enabled.reset(axis);
    #if HAS_TRINAMIC
    tmc_disable_stallguard(stepper_axis(axis), restore_stealth);
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    bsod("Unknown driver");
    #endif
}
#endif

void motor_driver_init() {
#if TMC_HAS_SPI
    #define SET_CS_PIN(st) OUT_WRITE(st##_CS_PIN, HIGH)
    SPI.begin();
    #if AXIS_HAS_SPI(X)
    SET_CS_PIN(X);
    #endif
    #if AXIS_HAS_SPI(Y)
    SET_CS_PIN(Y);
    #endif
    #if AXIS_HAS_SPI(Z)
    SET_CS_PIN(Z);
    #endif
    #if AXIS_HAS_SPI(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(Z2);
    #endif
    #if AXIS_HAS_SPI(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(Z3);
    #endif
    #if AXIS_HAS_SPI(E0)
    SET_CS_PIN(E0);
    #endif
    #if AXIS_HAS_SPI(E1)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(E1);
    #endif
    #if AXIS_HAS_SPI(E2)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(E2);
    #endif
    #if AXIS_HAS_SPI(E3)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(E3);
    #endif
    #if AXIS_HAS_SPI(E4)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(E4);
    #endif
    #if AXIS_HAS_SPI(E5)
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_CS_PIN(E5);
    #endif
#endif
#if HAS_TRINAMIC
    init_tmc();
#endif
}

void motor_driver_check_connections() {
#if HAS_TRINAMIC
    initial_test_tmc_connection();
#endif
}

void monitor_motor_drivers() {
    // currently done elsewhere
}

void motor_driver_loop() {
    // nothing to do
}

bool motor_check_coils(uint8_t axis) {
    return tmc_check_coils(axis);
}

void motor_prepare_move_xy() {
    // nothing to do
}

void motor_prepare_move_z() {
    // nothing to do
}

void motor_prepare_move_e() {
    // nothing to do
}

uint16_t stepper_microsteps(AxisEnum axis, std::optional<uint16_t> new_microsteps /* = std::nullopt */) {
    uint16_t result;

    switch (axis) {
#if AXIS_IS_TMC(X)
    case AxisEnum::X_AXIS:
        result = stepperX.microsteps();
        if (new_microsteps.has_value()) {
            stepperX.microsteps(new_microsteps.value());
        }
        break;
#endif
#if AXIS_IS_TMC(Y)
    case AxisEnum::Y_AXIS:
        result = stepperY.microsteps();
        if (new_microsteps.has_value()) {
            stepperY.microsteps(new_microsteps.value());
        }
        break;
#endif
#if AXIS_IS_TMC(Z)
    case AxisEnum::Z_AXIS:
        result = stepperZ.microsteps();
        if (new_microsteps.has_value()) {
            stepperZ.microsteps(new_microsteps.value());
        }
        break;
#endif
#if AXIS_IS_TMC(E0)
    case AxisEnum::E0_AXIS:
        result = stepperE0.microsteps();
        if (new_microsteps.has_value()) {
            stepperE0.microsteps(new_microsteps.value());
        }
        break;
#endif
    default:
        bsod("Unknown driver");
    }

    return std::max<uint16_t>(result, 1);
}

uint16_t stepper_mscnt(const AxisEnum axis) {
    switch (axis) {
#if AXIS_IS_TMC(X)
    case AxisEnum::X_AXIS:
        return stepperX.MSCNT();
#endif
#if AXIS_IS_TMC(Y)
    case AxisEnum::Y_AXIS:
        return stepperY.MSCNT();
        break;
#endif
#if AXIS_IS_TMC(Z)
    case AxisEnum::Z_AXIS:
        return stepperZ.MSCNT();
#endif
#if AXIS_IS_TMC(E0)
    case AxisEnum::E0_AXIS:
        return stepperE0.MSCNT();
#endif
    default:
        bsod("Unknown driver");
    }
}

bool stepper_wait_for_standstill(uint8_t axis_mask, millis_t max_delay) {
#if HAS_TRINAMIC
    millis_t timeout = millis() + max_delay;
    for (;;) {
        bool stst = true;
        LOOP_L_N(i, XYZE_N) {
            if (TEST(axis_mask, i)) {
                if (!stepper_axis((AxisEnum)i).stst()) {
                    stst = false;
                    break;
                }
            }
        }
        if (stst) {
            break;
        }
        if (millis() > timeout || TERN0(HAS_PLANNER(), planner.draining())) {
            return false;
        }
        safe_delay(10);
    }
#endif
    return true;
}

void restore_stepper_drivers() {
#if HAS_TRINAMIC
    restore_trinamic_drivers();
#endif
}

void reset_stepper_drivers() {
    SERIAL_ECHOPGM("Resetting stepper drivers\n");

#if HAS_DRIVER(TMC26X)
    // #error dead code found by automatic analyses (see BFW-5461)
    tmc26x_init_to_defaults();
#endif

#if HAS_TRINAMIC
    reset_trinamic_drivers();
#endif
}

/// Acquire lock for mutual exclusive access to the motor's serial port
///
/// With phase stepping, mutex is not an ideal synchronization mechanism as
/// high-priority ISR cannot safely take it. Since phase-stepping doesn't mind
/// missing transfers much, we can use atomic variable for storing the current
/// owner. As phase stepping holds the but all the time, we also have a flag that
/// marks that a task wants to access the bus.

osMutexDef(motor_mutex);
osMutexId motor_mutex_id;

static inline void motor_serial_lock_init_impl() {
    motor_mutex_id = osMutexCreate(osMutex(motor_mutex));
}

void motor_serial_lock_init() {
    motor_serial_lock_init_impl();
}

/// This implements required symbols declared within the TMCStepper library
extern "C" void tmc_serial_lock_init() {
    motor_serial_lock_init_impl();
}

enum class BusOwner {
    NOBODY = 0,
    TASK = 1,
    ISR = 2
};

static std::atomic<BusOwner> motor_bus_owner = BusOwner::NOBODY;
static std::atomic<bool> motor_bus_requested = false;
static std::atomic<bool> motor_bus_isr_starved = false;

/// Acquire lock for mutual exclusive access to the motor's serial port
static inline bool motor_serial_lock_acquire_impl() {
    if (osMutexWait(motor_mutex_id, osWaitForever) != osOK) {
        return false;
    }

    uint32_t start = ticks_ms();
    // We have taken the mutex, now let's try to take over the lock from ISR in
    // busy waiting. First, we do not want to starve the ISR, so we will wait
    // for it catch up from any hiccup caused by a previous lock acquisition in
    // a task. This shouldn't take longer than a number of axis ISR periods (~50
    // µs):
    motor_serial_lock_let_isr_run();

    // Then, we will try to atomically take the lock:
    BusOwner owner = BusOwner::NOBODY;
    motor_bus_requested = true;
    while (!motor_bus_owner.compare_exchange_weak(owner, BusOwner::TASK,
        std::memory_order_relaxed,
        std::memory_order_relaxed)) {
        if (ticks_diff(ticks_ms(), start) > 100) {
            bsod("Couldn't acquire motor bus within 100ms");
        }
        // When the SPI bus cannot be taken for some reason, the owner will remain in the owner variable
        // and we'll know if it was the ISR or another task. Therefore, owner is reset after the potential BSOD call above
        owner = BusOwner::NOBODY;
    }
    return true;
}

bool motor_serial_lock_acquire() {
    return motor_serial_lock_acquire_impl();
}

/// This implements required symbols declared within the TMCStepper library
extern "C" bool tmc_serial_lock_acquire() {
    return motor_serial_lock_acquire_impl();
}

/// Release lock for mutual exclusive access to the motor's serial port
static inline void motor_serial_lock_release_impl() {
    motor_bus_owner.store(BusOwner::NOBODY, std::memory_order_relaxed);
    motor_bus_requested = false;
    osMutexRelease(motor_mutex_id);
}

void motor_serial_lock_release() {
    return motor_serial_lock_release_impl();
}

/// This implements required symbols declared within the TMCStepper library
extern "C" void tmc_serial_lock_release() {
    return motor_serial_lock_release_impl();
}

bool motor_serial_lock_acquire_isr() {
    BusOwner owner = BusOwner::NOBODY;
    return motor_bus_owner.compare_exchange_weak(owner, BusOwner::ISR,
        std::memory_order_acquire,
        std::memory_order_relaxed);
}

void motor_serial_lock_release_isr() {
    motor_bus_owner.store(BusOwner::NOBODY, std::memory_order_release);
}

bool motor_serial_lock_held_by_isr() {
    return motor_bus_owner.load(std::memory_order_relaxed) == BusOwner::ISR;
}

void motor_serial_lock_let_isr_run() {
    int repetition = 0;
    while (motor_bus_isr_starved.load(std::memory_order_acquire)) {
        if (repetition++ > 100) {
            bsod("phstep starved");
        }
        delay_us(2);
    }
}

void motor_serial_lock_mark_isr_starved() {
    return motor_bus_isr_starved.store(true, std::memory_order_release);
}

void motor_serial_lock_clear_isr_starved() {
    return motor_bus_isr_starved.store(false, std::memory_order_release);
}

bool motor_serial_lock_held() {
    return uxSemaphoreGetCountFromISR(motor_mutex_id) == 0;
}

bool motor_serial_lock_requested_by_task() {
    return motor_bus_requested;
}

/// Called when an error occurs when communicating with the motor driver over serial
static inline void motor_communication_error_impl() {
    bsod("motor driver communication error");
}

/// This implements a required symbol declared within the TMCStepper library
void tmc_communication_error() {
    motor_communication_error_impl();
}
