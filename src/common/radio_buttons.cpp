#include "radio_buttons.hpp"

//define counts of individual radio buttons here
const RadioBtn RadioButtons::LoadUnloadButtons[RadioBtnCount<RadioBtnLoadUnload>()] = {
    {},                                       //Parking
    {},                                       //WaitingTemp,
    {},                                       //PreparingToRam,
    {},                                       //Ramming,
    {},                                       //Unloading,
    {},                                       //Unloading2,
    { Button::CONTINUE },                     //UserPush,
    {},                                       //MakeSureInserted,
    {},                                       //Inserting,
    {},                                       //Loading,
    {},                                       //Purging,
    {},                                       //Purging2,
    { Button::CONTINUE, Button::PURGE_MORE }, //IsColor,
    {},                                       //Purging3,
};

const RadioBtn RadioButtons::TestButtons[RadioBtnCount<RadioBtnTest>()] = {
    {},
    {}
};
