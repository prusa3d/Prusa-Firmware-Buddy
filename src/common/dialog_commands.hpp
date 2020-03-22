// every phase in dialog can have some buttons
// buttons are generalized on this level as commands
// because non GUI/WUI client can also use them

#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
enum { COMMAND_BITS = 2,
    MAX_COMMANDS = (1 << COMMAND_BITS) };

//list of all button types
enum class Command : uint8_t {
    _NONE = 0, //none must be zero becouse of empty initialization of array
    YES,
    NO,
    CONTINUE,
    OK,
    BACK,
    RETRY,
    PURGE_MORE,
    REHEAT
};

using PhaseCommands = std::array<Command, MAX_COMMANDS>;

//count cenum class members (if "_first" and "_last" is defined)
template <class T>
constexpr size_t CountPhases() {
    return static_cast<size_t>(T::_last) - static_cast<size_t>(T::_first) + 1;
}
//use this when creating an event
//encodes enum to position in Enem
template <class T>
constexpr uint8_t GetPhaseIndex(T phase) {
    return static_cast<size_t>(phase) - static_cast<size_t>(T::_first);
}

template <class T>
constexpr T GetEnumFromPhaseIndex(size_t index) {
    return static_cast<T>(static_cast<size_t>(T::_first) + index);
}

//define enum classes for commands here
//and YES phase can have 0 commands
//every enum must have "_first" and "_last"
//"_first" ==  "previous_enum::_last" + 1
//EVERY command shall have unique ID (so every button in GIU is unique)
enum class PhasesLoadUnload : uint16_t {
    _first = 0,
    Parking,
    WaitingTemp,
    PreparingToRam,
    Ramming,
    Unloading,
    Unloading2,
    UserPush,
    NozzleTimeout,
    MakeSureInserted,
    Inserting,
    Loading,
    Purging,
    Purging2,
    IsColor,
    Unparking,
    _last = Unparking
};

enum class PhasesTest : uint16_t {
    _first = static_cast<uint16_t>(PhasesLoadUnload::_last) + 1,
    Test1,
    Test2,
    _last = Test2
};

//static class for work with dialog commands
//encode commands - get them from marlin client, to marlin server and decode them again
class DialogCommands {
    DialogCommands() = delete;

    //declare 2d arrays of single buttons for radio buttons
    static const PhaseCommands LoadUnloadButtons[CountPhases<PhasesLoadUnload>()];
    static const PhaseCommands TestButtons[CountPhases<PhasesTest>()];

    //methods to "bind" button array with enum type
    static const PhaseCommands &getCommandsInPhase(PhasesLoadUnload phase) { return LoadUnloadButtons[static_cast<size_t>(phase)]; }
    static const PhaseCommands &getCommandsInPhase(PhasesTest phase) { return TestButtons[static_cast<size_t>(phase)]; }

protected:
    //get index of single command in PhaseCommands
    template <class T>
    static uint8_t GetIndex(T phase, Command command) {
        const PhaseCommands &cmds = getCommandsInPhase(phase);
        for (size_t i = 0; i < MAX_COMMANDS; ++i) {
            if (cmds[i] == command)
                return i;
        }
        return -1;
    }

    //get command from PhaseCommands by index
    template <class T>
    static Command GetCommand(T phase, uint8_t index) {
        if (index > MAX_COMMANDS)
            return Command::_NONE;
        const PhaseCommands &cmds = getCommandsInPhase(phase);
        return cmds[index];
    }

public:
    //get all commands for phase
    template <class T>
    static const PhaseCommands &GetCommands(T phase) {
        return getCommandsInPhase(phase);
    }

    //encode radio button and clicked index into int
    //use on client side
    template <class T>
    static uint32_t Encode(T phase, Command command) {
        uint8_t clicked_index = GetIndex(phase, command);
        if (clicked_index > MAX_COMMANDS)
            return -1; // this radio button does not have so many buttons
        return ((static_cast<uint32_t>(phase)) << COMMAND_BITS) + uint32_t(clicked_index);
    }
};
