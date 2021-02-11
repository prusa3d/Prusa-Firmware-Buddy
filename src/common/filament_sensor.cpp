/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

//there is 10kOhm PU to 5V in filament sensor
//MCU PU/PD is in range 30 - 50 kOhm

//when PD is selected and sensor is connected Vmcu = min 3.75V .. (5V * 30kOhm) / (30 + 10 kOhm)
//pin is 5V tolerant

//MCU has 5pF, transistor D-S max 15pF
//max R is 50kOhm
//Max Tau ~= 20*10^-12 * 50*10^3 = 1*10^-6 s ... about 1us

#include "filament_sensor.hpp"
#include "print_processor.hpp"
#include "fsensor_pins.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"

static volatile fsensor_t state = fsensor_t::NotInitialized;
static volatile fsensor_t last_state = fsensor_t::NotInitialized;

typedef enum {
    M600_on_edge = 0,
    M600_on_level = 1,
    M600_never = 2
} send_M600_on_t;

typedef struct {
    uint8_t M600_sent;
    uint8_t send_M600_on;
    uint8_t meas_cycle;
} status_t;
static status_t status = { 0, M600_on_edge, 0 };

static bool old_fs_state = false;
static bool run_first = true;
static bool current_detect_filament_insert = false;

/*---------------------------------------------------------------------------*/
//debug functions
int fs_was_M600_send() {
    return status.M600_sent != 0;
}

char fs_get_send_M600_on() {
    switch (status.send_M600_on) {
    case M600_on_edge:
        return 'e';
    case M600_on_level:
        return 'l';
    case M600_never:
        return 'n';
    default:
        return 'x';
    }
}

/*---------------------------------------------------------------------------*/
//local functions
static void _init();

//simple filter
//without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
static void _set_state(fsensor_t st) {
    CriticalSection C;
    if (last_state == st)
        state = st;
    last_state = st;
}

static void _enable() {
    FSensor::pullUp();
    state = fsensor_t::NotInitialized;
    last_state = fsensor_t::NotInitialized;
    status.meas_cycle = 0;
}

static void _disable() {
    state = fsensor_t::Disabled;
    last_state = fsensor_t::Disabled;
    status.meas_cycle = 0;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
fsensor_t fs_get_state() {
    return state;
}

//value can change during read, but it is not a problem
int fs_did_filament_runout() {
    return state == fsensor_t::NoFilament;
}

void fs_send_M600_on_edge() {
    CriticalSection C;
    status.send_M600_on = M600_on_edge;
}

void fs_send_M600_on_level() {
    CriticalSection C;
    status.send_M600_on = M600_on_level;
}

void fs_send_M600_never() {
    CriticalSection C;
    status.send_M600_on = M600_never;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
void fs_enable() {
    CriticalSection C;
    _enable();
    FSensorEEPROM::Set();
}

void fs_disable() {
    CriticalSection C;
    _disable();
    FSensorEEPROM::Clr();
}

uint8_t fs_get__send_M600_on__and_disable() {
    CriticalSection C;
    uint8_t ret = status.send_M600_on;
    status.send_M600_on = M600_never;
    return ret;
}
void fs_restore__send_M600_on(uint8_t send_M600_on) {
    CriticalSection C;
    //cannot call _init(); - it could cause stacking in uninitialized state
    status.send_M600_on = send_M600_on;
}

fsensor_t fs_wait_initialized() {
    fsensor_t ret = fs_get_state();
    while (ret == fsensor_t::NotInitialized) {
        Rtos::Delay(0); // switch to other threads
        ret = fs_get_state();
    }
    return ret;
}

void fs_clr_sent() {
    CriticalSection C;
    status.M600_sent = 0;
}

/*---------------------------------------------------------------------------*/
//global not thread safe functions
static void _init() {
    bool enabled = FSensorEEPROM::Get();

    if (enabled)
        _enable();
    else
        _disable();
}

void fs_init_on_edge() {
    _init();
    fs_send_M600_on_edge();
}
void fs_init_on_level() {
    _init();
    fs_send_M600_on_level();
}
void fs_init_never() {
    _init();
    fs_send_M600_never();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle
static void _injectM600() {
    if (status.M600_sent == 0 && (PrintProcessor::IsPrinting() && !PrintProcessor::IsM600injectionBlocked())) {
        PrintProcessor::InjectGcode("M600"); //change filament
        status.M600_sent = 1;
    }
}

static void _cycle0() {
    if (FSensor::Get()) {
        FSensor::pullDown();
        status.meas_cycle = 1; //next cycle shall be 1
    } else {
        int had_filament = state == fsensor_t::HasFilament ? 1 : 0;
        _set_state(fsensor_t::NoFilament); //it is filtered, 2 requests are needed to change state
        //M600_on_edge == inject after state was changed from HasFilament to NoFilament
        //M600_on_level == inject on NoFilament
        //M600_never == do not inject
        if (state == fsensor_t::NoFilament) {
            switch (status.send_M600_on) {
            case M600_on_edge:
                if (!had_filament)
                    break;
                // no break if had_filament == 1
            case M600_on_level:
                _injectM600();
                break;
            case M600_never:
            default:
                break;
            }
        }

        status.meas_cycle = 0; //remain in cycle 0
    }
}

//called only in fs_cycle
static void _cycle1() {
    //pulldown was set in cycle 0
    _set_state(FSensor::Get() ? fsensor_t::HasFilament : fsensor_t::NotConnected);
    FSensor::pullUp();
    status.meas_cycle = 0; //next cycle shall be 0
}

static void _autoload_loop() {
    if (PrintProcessor::IsAutoloadEnabled()) {
        if (fs_get_state() == fsensor_t::HasFilament) {
            current_detect_filament_insert = true;
        } else if (fs_get_state() == fsensor_t::NoFilament) {
            current_detect_filament_insert = false;
            run_first = false;
        }

        if (current_detect_filament_insert != old_fs_state && current_detect_filament_insert == true && run_first == false) {
            if (!PrintProcessor::IsPrinting()) {
                PrintProcessor::InjectGcode("M1400 S1");
            }
        }

        old_fs_state = current_detect_filament_insert;
    }
}

//delay between calls must be 1us or longer
void fs_cycle() {
    //sensor is disabled (only init can enable it)
    if (state == fsensor_t::Disabled)
        return;

    PrintProcessor::Update();

    //sensor is enabled
    if (status.meas_cycle == 0) {
        _cycle0();
    } else {
        _cycle1();
    }

    _autoload_loop();
}
