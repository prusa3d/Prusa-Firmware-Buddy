#include "dialog_commands.hpp"

//define available commands for every phase
const PhaseCommands DialogCommands::LoadUnloadButtons[CountPhases<PhasesLoadUnload>()] = {
    {},                                         //_init
    {},                                         //Parking
    {},                                         //WaitingTemp,
    {},                                         //PreparingToRam,
    {},                                         //Ramming,
    {},                                         //Unloading,
    {},                                         //Unloading2,
    { Command::Continue },                      //UserPush,
    { Command::Reheat },                        //NozzleTimeout,
    {},                                         //MakeSureInserted,
    {},                                         //Inserting,
    {},                                         //Loading,
    {},                                         //Purging,
    {},                                         //Purging2,
    { Command::Continue, Command::Purge_more }, //IsColor,
    {},                                         //Unparking,
};

const PhaseCommands DialogCommands::TestButtons[CountPhases<PhasesTest>()] = {
    {},
    {},
    {}
};
