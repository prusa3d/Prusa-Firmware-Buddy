#include "client_response.hpp"

//define available responses for every phase
const PhaseResponses ClientResponses::LoadUnloadResponses[CountPhases<PhasesLoadUnload>()] = {
    {},                                                            //_first
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
    { Response::Continue, Response::Purge_more },                  //IsColorPurge
    {},                                                            //Unparking,
};

const PhaseResponses ClientResponses::G162Responses[CountPhases<PhasesG162>()] = {
    {}, //_first
    {}, //Parking
};

const PhaseResponses ClientResponses::SelfTestResponses[CountPhases<PhasesSelfTest>()] = {
    {}, //_first
    {}, //FansAxis
    {}, //Heaters
    {}, //Cooldown
};
/*
const PhaseResponses ClientResponses::SelfTestHeatersResponses[CountPhases<PhasesSelfTestHeaters>()] = {
    {}, //_first
    {}, //Init
};
*/
