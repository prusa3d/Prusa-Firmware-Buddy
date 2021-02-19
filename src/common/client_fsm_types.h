#pragma once

#include <stdint.h>

#ifdef __cplusplus
//C++ checks enum classes

//Client finite state machines
enum class ClientFSM : uint8_t {
    Serial_printing,
    Load_unload,
    Preheat,
    G162,
    SelftestAxis,
    SelftestFans,
    SelftestHeat,
    Printing, //not a dialog
    FirstLayer,
    _none, //cannot be created, must have same index as _count
    _count = _none
};

enum class ClientFSM_Command : uint8_t {
    none = 0x00,
    create = 0x80,
    destroy = 0x40,
    change = create | destroy,
    _mask = change
};

enum class LoadUnloadMode : uint8_t {
    Change,
    Load,
    Unload,
    Purge
};

enum class PreheatMode : uint8_t {
    None,
    Load,
    Unload,
    Purge,
    Change_phase1, // do unload, call Change_phase2 after load finishes
    Change_phase2, // do load, meant to be used recursively in Change_phase1
    _last = Change_phase2
};

enum class RetAndCool_t {
    Neither,
    Return,
    Cooldown,
    Both
};

class PreheatData {
    uint8_t mode : 3;
    bool has_return_option : 1;
    bool has_cooldown_option : 1;

public:
    constexpr PreheatData(uint8_t data)
        : mode((data & 0x07) <= uint8_t(PreheatMode::_last) ? data & 0x07 : uint8_t(PreheatMode::None))
        , has_return_option(data & (1 << 6))
        , has_cooldown_option((data & (1 << 7)) && Mode() == PreheatMode::None) {}
    constexpr PreheatData(PreheatMode mode, RetAndCool_t ret_cool)
        : mode(uint8_t(mode))
        , has_return_option(ret_cool == RetAndCool_t::Return || ret_cool == RetAndCool_t::Both)
        , has_cooldown_option((ret_cool == RetAndCool_t::Cooldown || ret_cool == RetAndCool_t::Both) && Mode() == PreheatMode::None) {}

    constexpr PreheatMode Mode() const { return PreheatMode(mode); }
    constexpr bool HasReturnOption() const { return has_return_option; }
    constexpr bool HasCooldownOption() const { return has_cooldown_option; }
    constexpr RetAndCool_t RetAndCool() {
        if (HasReturnOption() && HasCooldownOption())
            return RetAndCool_t::Both;
        if (HasReturnOption())
            return RetAndCool_t::Return;
        if (HasCooldownOption())
            return RetAndCool_t::Cooldown;
        return RetAndCool_t::Neither;
    }
    constexpr uint8_t Data() const {
        uint8_t ret = mode;
        ret |= uint8_t(has_return_option) << 6;
        ret |= uint8_t(has_cooldown_option) << 7;
        return ret;
    }
};

static_assert(sizeof(PreheatData) == 1, "Error PreheatData is too big");

enum class WarningType : uint32_t {
    HotendFanError,
    PrintFanError,
    HeatersTimeout,
    NozzleTimeout,
    USBFlashDiskError,
    _last = USBFlashDiskError
};

// Open dialog has a parameter because I need to set a caption of change filament dialog (load / unload / change).
// Use extra state of statemachine to set the caption would be cleaner, but I can miss events.
// Only the last sent event is guaranteed to pass its data.
using fsm_cb_t = void (*)(uint32_t); //create/destroy/change finite state machine
using message_cb_t = void (*)(const char *);
using warning_cb_t = void (*)(WarningType);
using startup_cb_t = void (*)(void);
#else  // !__cplusplus
//C
typedef void (*fsm_cb_t)(uint32_t); //create/destroy/change finite state machine
typedef void (*message_cb_t)(const char *);
typedef void (*warning_cb_t)(uint32_t);
typedef void (*startup_cb_t)(void);
#endif //__cplusplus
