#pragma once

#include <cstdint>
#include <cstddef>
//count cenum class members (if "_first" and "_last" is defined)
template <class T>
constexpr size_t RadioBtnCount() {
    return static_cast<size_t>(T::_last) - static_cast<size_t>(T::_first);
}

//define enum classes for buttons here
//every enum must have "_first" and "_last"
//"_first" ==  "previous_enum::_last" + 1
//also "NoBtn" should be defined with same value as "_last" (for future use)
enum class RadioBtnLoadUnload : uint16_t {
    _first = 0,
    bt1 = _first,
    bt2,
    NoBtn, //usea sa both couat and no button clicked
    _last = NoBtn
};

enum class RadioBtnTest : uint16_t {
    _first = static_cast<uint16_t>(RadioBtnLoadUnload::_last) + 1,
    Test1 = _first,
    Test2,
    NoBtn, //usea sa both couat and no button clicked
    _last = NoBtn
};

//static class for work with radiobuttons
class RadioButtons {
    RadioButtons() = delete;

    //declare counts of individual radio buttons here
    static const uint8_t LoadUnloadCounts[RadioBtnCount<RadioBtnLoadUnload>()];
    static const uint8_t TestCounts[RadioBtnCount<RadioBtnTest>()];

public:
    enum { MAX_BTNS = 4 };

    static uint8_t GetCount(RadioBtnLoadUnload bt) {
        return LoadUnloadCounts[static_cast<size_t>(bt)];
    }

    static uint8_t GetCount(RadioBtnTest bt) {
        return TestCounts[static_cast<size_t>(bt)];
    }

    //encode radio button and clicked index into int
    template <class T>
    static uint32_t Encode(T bt, uint8_t clicked_index) {
        if (clicked_index >= MAX_BTNS)
            return -1; //button num is 0-3 (1 - 4 buttons)
        if (bt == T::NoBtn_Count)
            return -1; //count cannot be used
        if (clicked_index >= GetCount(bt))
            return -1; // this radio button does not have so many buttons
        return ((static_cast<uint32_t>(bt)) << 2) + uint32_t(clicked_index);
    }
};
