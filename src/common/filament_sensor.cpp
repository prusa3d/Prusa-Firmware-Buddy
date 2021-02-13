/**
 * @file filament_sensor.cpp
 * @author Radek Vana
 * @date 2019-12-16
 */

#include "filament_sensor.hpp"
#include "print_processor.hpp"
#include "fsensor_eeprom.hpp"
#include "rtos_api.hpp"

FSensor::FSensor()
    : state(fsensor_t::NotInitialized)
    , last_state(fsensor_t::NotInitialized)
    , event_lock(0) {
    PrintProcessor::Init();
}

/*---------------------------------------------------------------------------*/
//debug functions
bool FSensor::WasM600_send() {
    return status.M600_sent;
}

char FSensor::GetM600_send_on() {
    switch (status.send_event_on) {
    case inject_t::on_edge:
        return 'e';
    case inject_t::on_level:
        return 'l';
    case inject_t::never:
        return 'n';
    }
    return 'x';
}

//simple filter
//without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
void FSensor::set_state(fsensor_t st) {
    CriticalSection C;
    if (last_state == st)
        state = st;
    last_state = st;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
fsensor_t FSensor::Get() {
    return state;
}

//value can change during read, but it is not a problem
bool FSensor::DidRunOut() {
    return state == fsensor_t::NoFilament;
}

void FSensor::M600_on_edge() {
    CriticalSection C;
    status.send_event_on = inject_t::on_edge;
}

void FSensor::M600_on_level() {
    CriticalSection C;
    status.send_event_on = inject_t::on_level;
}

void FSensor::M600_never() {
    CriticalSection C;
    status.send_event_on = inject_t::never;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
void FSensor::Enable() {
    CriticalSection C;
    enable();
    FSensorEEPROM::Set();
}

void FSensor::Disable() {
    CriticalSection C;
    disable();
    FSensorEEPROM::Clr();
}

FSensor::inject_t FSensor::getM600_send_on_and_disable() {
    CriticalSection C;
    inject_t ret = status.send_event_on;
    status.send_event_on = inject_t::never;
    return ret;
}
void FSensor::restore_send_M600_on(FSensor::inject_t send_event_on) {
    CriticalSection C;
    //cannot call init(); - it could cause stacking in uninitialized state
    status.send_event_on = send_event_on;
}

fsensor_t FSensor::WaitInitialized() {
    fsensor_t ret = FSensor::Get();
    while (ret == fsensor_t::NotInitialized) {
        Rtos::Delay(0); // switch to other threads
        ret = FSensor::Get();
    }
    return ret;
}

void FSensor::ClrM600Sent() {
    CriticalSection C;
    status.M600_sent = false;
}

void FSensor::ClrAutoloadSent() {
    CriticalSection C;
    status.Autoload_sent = false;
}

uint32_t FSensor::DecEvLock() {
    CriticalSection C;
    if (event_lock > 0)
        --event_lock;
    return event_lock;
}

uint32_t FSensor::IncEvLock() {
    CriticalSection C;
    return ++event_lock;
}

/*---------------------------------------------------------------------------*/
//global not thread safe functions
void FSensor::init() {
    bool enabled = FSensorEEPROM::Get();

    if (enabled)
        enable();
    else
        disable();
}

void FSensor::InitOnEdge() {
    init();
    FSensor::M600_on_edge();
}
void FSensor::InitOnLevel() {
    init();
    FSensor::M600_on_level();
}
void FSensor::InitNever() {
    init();
    FSensor::M600_never();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle

//M600_on_edge == inject after state was changed from HasFilament to NoFilament
//M600_on_level == inject on NoFilament
//M600_never == do not inject
void FSensor::evaluateEventConditions(event ev) {
    if (!isEvLocked()) {
        if (PrintProcessor::IsPrinting()) {
            //M600
            if (!status.M600_sent) {
                if ((status.send_event_on == inject_t::on_edge && ev == event::EdgeFilamentRemoved) || (status.send_event_on == inject_t::on_level && ev == event::NoFilament)) {
                    PrintProcessor::InjectGcode("M600"); //change filament
                    status.M600_sent = true;
                }
            }
        } else {
            //autoload
            if (PrintProcessor::IsAutoloadEnabled() && (!status.Autoload_sent)) {
                if ((status.send_event_on == inject_t::on_edge && ev == event::EdgeFilamentInserted) || (status.send_event_on == inject_t::on_level && ev == event::HasFilament)) {
                    PrintProcessor::InjectGcode("M1400 S65"); //load with return option
                    status.Autoload_sent = true;
                }
            }
        }
    }
}

FSensor::event FSensor::generateEvent(fsensor_t last_state_before_cycle) const {
    bool has_filament = state == fsensor_t::HasFilament;
    if (last_state_before_cycle == fsensor_t::HasFilament) {
        if (has_filament) {
            //had && has
            return event::HasFilament;
        } else {
            //had && !has
            return event::EdgeFilamentRemoved;
        }
    } else if (last_state_before_cycle == fsensor_t::NoFilament) {
        if (has_filament) {
            //!had && has
            return event::EdgeFilamentInserted;
        } else {
            //!had && !has
            return event::NoFilament;
        }
    }

    return has_filament ? event::HasFilament : event::NoFilament; // state changed, but NO edge detected, most likely change from init state
}

//delay between calls must be 1us or longer
void FSensor::Cycle() {
    //sensor is disabled (only init can enable it)
    if (state == fsensor_t::Disabled)
        return;

    PrintProcessor::Update();

    fsensor_t last_state_before_cycle = state;

    cycle();

    event ev = generateEvent(last_state_before_cycle);
    evaluateEventConditions(ev);
}
