/**
 * @file filament_sensor.hpp
 * @author Radek Vana
 * @brief basic api of filament sensor
 * @date 2019-12-16
 */

#pragma once

#include "stdint.h"

enum class fsensor_t : uint8_t {
    NotInitialized, //enable enters this state too
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};

//basic filament sensor api
class FSensor {
protected:
    enum class event {
        NoFilament,
        HasFilament,
        EdgeFilamentInserted,
        EdgeFilamentRemoved
    };

    enum class inject_t : uint8_t {
        on_edge = 0,
        on_level = 1,
        never = 2
    };

    struct status_t {
        bool M600_sent;
        bool Autoload_sent;
        inject_t send_event_on;

        status_t()
            : M600_sent(false)
            , Autoload_sent(false)
            , send_event_on(inject_t::on_edge) {}
    };
    status_t status;

    volatile fsensor_t state;
    volatile fsensor_t last_state;

    uint32_t event_lock; // 0 == unlocked

    void init();
    void set_state(fsensor_t st);

    void restore_send_M600_on(FSensor::inject_t send_event_on);
    inject_t getM600_send_on_and_disable();

    event generateEvent(fsensor_t last_state_before_cycle) const;
    void evaluateEventConditions(event ev);

    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void cycle() = 0;

    inline bool isEvLocked() { return event_lock > 0; }

public:
    void Cycle();
    //thread safe functions
    fsensor_t Get();
    bool DidRunOut(); //for arduino / marlin

    //switch behavior when M600 should be send
    void M600_on_edge(); //default behavior
    void M600_on_level();
    void M600_never();

    //thread safe functions, but cannot be called from interrupt
    void Enable();
    void Disable();

    uint32_t DecEvLock();
    uint32_t IncEvLock();

    fsensor_t WaitInitialized();
    void ClrM600Sent();
    void ClrAutoloadSent();

    //not thread safe functions
    void InitOnEdge();
    void InitOnLevel();
    void InitNever();

    //for debug
    bool WasM600_send();
    char GetM600_send_on();

    FSensor();
};

//singleton defined in childs cpp file
FSensor &FS_instance();
