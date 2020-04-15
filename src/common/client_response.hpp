// every phase in dialog can have some buttons
// buttons are generalized on this level as responses
// because non GUI/WUI client can also use them

#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

enum { RESPONSE_BITS = 2,                   //number of bits used to encode response
    MAX_RESPONSES = (1 << RESPONSE_BITS) }; //maximum number of responses in one phase

//list of all button types
enum class Response : uint8_t {
    _none = 0, //none must be zero because of empty initialization of array
    Yes,
    No,
    Continue,
    Ok,
    Back,
    Retry,
    Purge_more,
    Reheat
};

using PhaseResponses = std::array<Response, MAX_RESPONSES>;

//count enum class members (if "_first" and "_last" is defined)
template <class T>
constexpr size_t CountPhases() {
    return static_cast<size_t>(T::_last) - static_cast<size_t>(T::_first) + 1;
}
//use this when creating an event
//encodes enum to position in phase
template <class T>
constexpr uint8_t GetPhaseIndex(T phase) {
    return static_cast<size_t>(phase) - static_cast<size_t>(T::_first);
}

template <class T>
constexpr T GetEnumFromPhaseIndex(size_t index) {
    return static_cast<T>(static_cast<size_t>(T::_first) + index);
}

//define enum classes for responses here
//and YES phase can have 0 responses
//every enum must have "_first" and "_last"
//"_first" ==  "previous_enum::_last" + 1
//EVERY response shall have unique ID (so every button in GIU is unique)
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

//static class for work with fsm responses (like button click)
//encode responses - get them from marlin client, to marlin server and decode them again
class ClientResponses {
    ClientResponses() = delete;
    ClientResponses(ClientResponses &) = delete;

    //declare 2d arrays of single buttons for radio buttons
    static const PhaseResponses LoadUnloadResponses[CountPhases<PhasesLoadUnload>()];
    static const PhaseResponses TestResponses[CountPhases<PhasesTest>()];

    //methods to "bind" button array with enum type
    static const PhaseResponses &getResponsesInPhase(PhasesLoadUnload phase) { return LoadUnloadResponses[static_cast<size_t>(phase)]; }
    static const PhaseResponses &getResponsesInPhase(PhasesTest phase) { return TestResponses[static_cast<size_t>(phase)]; }

protected:
    //get index of single response in PhaseResponses
    //return -1 (maxval) if does not exist
    template <class T>
    static uint8_t GetIndex(T phase, Response response) {
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        for (size_t i = 0; i < MAX_RESPONSES; ++i) {
            if (cmds[i] == response)
                return i;
        }
        return -1;
    }

    //get response from PhaseResponses by index
    template <class T>
    static Response GetResponse(T phase, uint8_t index) {
        if (index > MAX_RESPONSES)
            return Response::_none;
        const PhaseResponses &cmds = getResponsesInPhase(phase);
        return cmds[index];
    }

public:
    //get all responses accepted in phase
    template <class T>
    static const PhaseResponses &GetResponses(T phase) {
        return getResponsesInPhase(phase);
    }

    //encode phase and client response (in GUI radio button and clicked index) into int
    //use on client side
    //return -1 (maxval) if does not exist
    template <class T>
    static uint32_t Encode(T phase, Response response) {
        uint8_t clicked_index = GetIndex(phase, response);
        if (clicked_index > MAX_RESPONSES)
            return -1; // this phase does not have response with this index
        return ((static_cast<uint32_t>(phase)) << RESPONSE_BITS) + uint32_t(clicked_index);
    }
};
