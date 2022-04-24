#pragma once

#include <stdint.h>

#ifdef __cplusplus
//C++ checks enum classes

// Client finite state machines
// bound to src/common/client_response.hpp
enum class ClientFSM : uint8_t {
    Serial_printing,
    Load_unload,
    Preheat,
    Selftest,
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
    Unload_askUnloaded,
    Autoload,
    _last = Autoload
};

enum class RetAndCool_t {
    Neither,
    Cooldown,
    Return,
    Both,
    last_ = Both
};

class PreheatData {
    static constexpr unsigned mode_digits = 4;
    static constexpr uint8_t mode_mask = (1 << mode_digits) - 1;
    static constexpr uint8_t return_option_digit_offset = 6;
    static constexpr uint8_t return_option_mask = 1 << return_option_digit_offset;
    static constexpr uint8_t cooldown_option_digit_offset = 7;
    static constexpr uint8_t cooldown_option_mask = 1 << cooldown_option_digit_offset;
    uint8_t mode : mode_digits;
    bool has_return_option : 1;
    bool has_cooldown_option : 1;

public:
    constexpr PreheatData(PreheatMode mode, RetAndCool_t ret_cool = RetAndCool_t::Neither)
        : mode(uint8_t(mode))
        , has_return_option(ret_cool == RetAndCool_t::Return || ret_cool == RetAndCool_t::Both)
        , has_cooldown_option((ret_cool == RetAndCool_t::Cooldown || ret_cool == RetAndCool_t::Both) && Mode() == PreheatMode::None) {}
    constexpr PreheatData(uint8_t data)
        : PreheatData(GetMode(data), GetRetAndCool(data)) {}

    constexpr PreheatMode Mode() const { return PreheatMode(mode); }
    constexpr bool HasReturnOption() const { return has_return_option; }
    constexpr bool HasCooldownOption() const { return has_cooldown_option; }
    constexpr RetAndCool_t RetAndCool() {
        return GetRetAndCool(Data());
    }
    constexpr uint8_t Data() const {
        uint8_t ret = mode;
        ret |= uint8_t(has_return_option) << return_option_digit_offset;
        ret |= uint8_t(has_cooldown_option) << cooldown_option_digit_offset;
        return ret;
    }

    // conversionfunctions for ctors etc
    static constexpr bool GetReturnOption(uint8_t data) {
        return data & return_option_mask;
    }
    static constexpr bool GetCooldownOption(uint8_t data) {
        return data & cooldown_option_mask;
    }
    static constexpr RetAndCool_t GetRetAndCool(uint8_t data) {
        const bool has_ret = GetReturnOption(data);
        const bool has_cool = GetCooldownOption(data);
        if (has_ret && has_cool)
            return RetAndCool_t::Both;
        if (has_ret)
            return RetAndCool_t::Return;
        if (has_cool)
            return RetAndCool_t::Cooldown;
        return RetAndCool_t::Neither;
    }
    static constexpr PreheatMode GetMode(uint8_t data) {
        return PreheatMode((data & mode_mask) <= uint8_t(PreheatMode::_last) ? data & mode_mask : uint8_t(PreheatMode::None));
    }
};

static_assert(sizeof(PreheatData) == 1, "Error PreheatData is too big");

enum class WarningType : uint32_t {
    HotendFanError,
    PrintFanError,
    HeatersTimeout,
    HotendTempDiscrepancy,
    NozzleTimeout,
    USBFlashDiskError,
    _last = USBFlashDiskError
};

// Open dialog has a parameter because I need to set a caption of change filament dialog (load / unload / change).
// Use extra state of statemachine to set the caption would be cleaner, but I can miss events.
// Only the last sent event is guaranteed to pass its data.
using fsm_cb_t = void (*)(uint32_t, uint16_t); //create/destroy/change finite state machine
using message_cb_t = void (*)(const char *);
using warning_cb_t = void (*)(WarningType);
using startup_cb_t = void (*)(void);
#else  // !__cplusplus
//C
typedef void (*fsm_cb_t)(uint32_t, uint16_t); //create/destroy/change finite state machine
typedef void (*message_cb_t)(const char *);
typedef void (*warning_cb_t)(uint32_t);
typedef void (*startup_cb_t)(void);
#endif //__cplusplus
