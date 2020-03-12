#pragma once

#include <cstdint>
#include <cstddef>
//count cenum class members (if "_first" and "_last" is defined)
template <class T>
constexpr size_t RadioBtnCount() {
    return static_cast<size_t>(T::_last) - static_cast<size_t>(T::_first) + 1;
}
//use this when creating an event
template <class T>
constexpr uint8_t PhaseFromRadioBtn(T btn) {
    return static_cast<size_t>(btn) - static_cast<size_t>(T::_first);
}

//define enum classes for buttons here
//and YES radio button can have 0 buttons
//every enum must have "_first" and "_last"
//"_first" ==  "previous_enum::_last" + 1
//EVERY button shall have unique ID
enum class RadioBtnLoadUnload : uint16_t {
    _first = 0,
    Parking = _first,
    WaitingTemp,
    PreparingToRam,
    Ramming,
    Unloading,
    Unloading2,
    UserPush,
    MakeSureInserted,
    Inserting,
    Loading,
    Purging,
    Purging2,
    IsColor,
    Purging3,
    _last = Purging3
};

enum class RadioBtnTest : uint16_t {
    _first = static_cast<uint16_t>(RadioBtnLoadUnload::_last) + 1,
    Test1 = _first,
    Test2,
    _last = Test2
};

//static class for work with radiobuttons
//encode buttons - get them from marlin client, to marlin server and decode them again
class RadioButtons {
    RadioButtons() = delete;

    //declare counts of individual radio buttons here
    static const uint8_t LoadUnloadCounts[RadioBtnCount<RadioBtnLoadUnload>()];
    static const uint8_t TestCounts[RadioBtnCount<RadioBtnTest>()];

public:
    enum { BTNS_BITS = 2,
        MAX_BTNS = (1 << BTNS_BITS) };

    static uint8_t GetCount(RadioBtnLoadUnload bt) {
        return LoadUnloadCounts[static_cast<size_t>(bt)];
    }

    static uint8_t GetCount(RadioBtnTest bt) {
        return TestCounts[static_cast<size_t>(bt)];
    }

    //encode radio button and clicked index into int
    //use on client side
    template <class T>
    static uint32_t Encode(T bt, uint8_t clicked_index) {
        if (clicked_index >= MAX_BTNS)
            return -1; //button num is 0-3 (1 - 4 buttons)
        if (bt == T::NoBtn_Count)
            return -1; //count cannot be used
        if (clicked_index >= GetCount(bt))
            return -1; // this radio button does not have so many buttons
        return ((static_cast<uint32_t>(bt)) << BTNS_BITS) + uint32_t(clicked_index);
    }
};
