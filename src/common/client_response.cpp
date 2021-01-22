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
    { Response::Yes, Response::No },                               //IsFilamentUnloaded,
    { Response::Continue },                                        //ManualUnload,
    { Response::Continue },                                        //UserPush,
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

const PhaseResponses ClientResponses::PreheatResponses[CountPhases<PhasesPreheat>()] = {
    {},                                                                                                                                                               //_first
    { Response::Abort, Response::Cooldown, Response::PLA, Response::PETG, Response::ASA, Response::ABS, Response::PC, Response::FLEX, Response::HIPS, Response::PP }, //UserTempSelection
};

const PhaseResponses ClientResponses::G162Responses[CountPhases<PhasesG162>()] = {
    {}, //_first
    {}, //Parking
};
