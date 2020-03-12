#include "radio_buttons.hpp"

//define counts of individual radio buttons here
const uint8_t RadioButtons::LoadUnloadCounts[RadioBtnCount<RadioBtnLoadUnload>()] = {
    0, //Parking
    0, //WaitingTemp,
    0, //PreparingToRam,
    0, //Ramming,
    0, //Unloading,
    0, //Unloading2,
    1, //UserPush,
    0, //MakeSureInserted,
    0, //Inserting,
    0, //Loading,
    0, //Purging,
    0, //Purging2,
    0, //IsColor,
    0, //Purging3,
};

const uint8_t RadioButtons::TestCounts[RadioBtnCount<RadioBtnTest>()] = {
    1,
    2
};
