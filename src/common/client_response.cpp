#include "client_response.hpp"

//define available responses for every phase
const PhaseResponses ClientResponses::LoadUnloadResponses[CountPhases<PhasesLoadUnload>()] = {
    {},                                           //_init
    {},                                           //Parking
    {},                                           //WaitingTemp,
    {},                                           //PreparingToRam,
    {},                                           //Ramming,
    {},                                           //Unloading,
    {},                                           //Unloading2,
    { Response::Continue },                       //UserPush,
    { Response::Reheat },                         //NozzleTimeout,
    {},                                           //MakeSureInserted,
    {},                                           //Inserting,
    {},                                           //Loading,
    {},                                           //Purging,
    {},                                           //Purging2,
    { Response::Continue, Response::Purge_more }, //IsColor,
    {},                                           //Unparking,
};

const PhaseResponses ClientResponses::TestResponses[CountPhases<PhasesTest>()] = {
    {},
    {},
    {}
};
