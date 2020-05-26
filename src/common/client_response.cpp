#include "client_response.hpp"

//define available responses for every phase
const PhaseResponses ClientResponses::LoadUnloadResponses[CountPhases<PhasesLoadUnload>()] = {
    {},                                                            //_init
    {},                                                            //Parking
    {},                                                            //WaitingTemp,
    {},                                                            //PreparingToRam,
    {},                                                            //Ramming,
    {},                                                            //Unloading,
    { Response::Filament_removed },                                //RemoveFilament,
    { Response::Continue },                                        //UserPush,
    { Response::Reheat },                                          //NozzleTimeout,
    {},                                                            //MakeSureInserted,
    {},                                                            //Inserting,
    { Response::Yes, Response::No },                               //IsFilamentInGear,
    {},                                                            //Ejecting,
    {},                                                            //Loading,
    {},                                                            //Purging,
    { Response::Continue, Response::Purge_more, Response::Retry }, //IsColor,
    {},                                                            //Unparking,
};

const PhaseResponses ClientResponses::TestResponses[CountPhases<PhasesTest>()] = {
    {},
    {},
    {}
};
