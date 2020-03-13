#pragma once

#include <cstdint>
#include <cstddef>

enum { BTNS_BITS = 2,
    MAX_BTNS = (1 << BTNS_BITS) };

//list of all button types
enum class Button : uint8_t {
    _NONE = 0, //none must be zero becouse of empty initialization of array
    YES,
    NO,
    CONTINUE,
    OK,
    BACK,
    RETRY,
    PURGE_MORE
};

using RadioBtn = Button[MAX_BTNS];

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
//EVERY radio button shall have unique ID
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

    //declare 2d arrays of single buttons for radio buttons
    static const RadioBtn LoadUnloadButtons[RadioBtnCount<RadioBtnLoadUnload>()];
    static const RadioBtn TestButtons[RadioBtnCount<RadioBtnTest>()];

    //methods to "bind" button array with enum type
    static const Button *getBtns(RadioBtnLoadUnload radio_bt) { return &(LoadUnloadButtons[static_cast<size_t>(radio_bt)][0]); }
    static const Button *getBtns(RadioBtnTest radio_bt) { return &(TestButtons[static_cast<size_t>(radio_bt)][0]); }

public:
    //get index of single button in radiobutton
    template <class T>
    static uint8_t GetIndex(T radio_bt, Button btn) {
        const Button *pBtns = getBtns(radio_bt);
        for (size_t i = 0; i < MAX_BTNS; ++i) {
            if (pBtns[i] == btn)
                return i;
        }
        return -1;
    }

    //get button by index
    template <class T>
    static uint8_t GetButton(T radio_bt, uint8_t index) {
        if (index > MAX_BTNS)
            return Button::_NONE;
        const Button *pBtns = getBtns(radio_bt);
        return pBtns[index];
    }

    //encode radio button and clicked index into int
    //use on client side
    template <class T>
    static uint32_t Encode(T radio_bt, Button btn) {
        if (radio_bt == T::NoBtn_Count)
            return -1; //count cannot be used
        uint8_t clicked_index = GetIndex(radio_bt);
        if (clicked_index > MAX_BTNS)
            return -1; // this radio button does not have so many buttons
        return ((static_cast<uint32_t>(radio_bt)) << BTNS_BITS) + uint32_t(clicked_index);
    }
};
