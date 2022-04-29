#include "client_response.hpp"

//define available responses for every phase
const PhaseResponses ClientResponses::LoadUnloadResponses[CountPhases<PhasesLoadUnload>()] = {
    {},                                                            //_first
    { Response::Stop },                                            //Parking_stoppable
    {},                                                            //Parking_unstoppable,
    { Response::Stop },                                            //WaitingTemp_stoppable,
    {},                                                            //WaitingTemp_unstoppable,
    { Response::Stop },                                            //PreparingToRam_stoppable,
    {},                                                            //PreparingToRam_unstoppable
    { Response::Stop },                                            //Ramming_stoppable,
    {},                                                            //Ramming_unstoppable,
    { Response::Stop },                                            //Unloading_stoppable,
    {},                                                            //Unloading_unstoppable,
    { Response::Filament_removed },                                //RemoveFilament,
    { Response::Yes, Response::No },                               //IsFilamentUnloaded,
    {},                                                            //FilamentNotInFS
    { Response::Continue },                                        //ManualUnload,
    { Response::Continue, Response::Stop },                        //UserPush_stoppable,
    { Response::Continue },                                        //UserPush_unstoppable,
    { Response::Stop },                                            //MakeSureInserted_stoppable,
    {},                                                            //MakeSureInserted_unstoppable,
    { Response::Stop },                                            //Inserting_stoppable,
    {},                                                            //Inserting_unstoppable,
    { Response::Yes, Response::No },                               //IsFilamentInGear,
    { Response::Stop },                                            //Ejecting_stoppable,
    {},                                                            //Ejecting_unstoppable,
    { Response::Stop },                                            //Loading_stoppable,
    {},                                                            //Loading_unstoppable,
    { Response::Stop },                                            //Purging_stoppable,
    {},                                                            //Purging_unstoppable,
    { Response::Continue, Response::Purge_more, Response::Retry }, //IsColor,
    { Response::Continue, Response::Purge_more },                  //IsColorPurge
    {},                                                            //Unparking,
};

const PhaseResponses ClientResponses::PreheatResponses[CountPhases<PhasesPreheat>()] = {
    {}, //_first
    { Response::Abort, Response::Cooldown, Response::PLA, Response::PETG,
        Response::ASA, Response::ABS, Response::PC, Response::FLEX, Response::HIPS, Response::PP, Response::PVB }, //UserTempSelection
};

const PhaseResponses ClientResponses::SelftestResponses[CountPhases<PhasesSelftest>()] = {
    {},                                      // _first
    { Response::Continue, Response::Abort }, // ESP_ask_auto
    { Response::Continue, Response::Abort }, // ESP_ask_from_menu
    { Response::Abort },                     // ESP_upload
    { Response::Continue },                  // ESP_passed
    { Response::Continue },                  // ESP_failed
    { Response::Continue, Response::Abort }, // ESP_credentials_instructions
    { Response::Continue, Response::Abort }, // ESP_credentials_instructions_stand_alone
    { Response::Continue, Response::Abort }, // ESP_credentials_ask_gen
    { Response::Continue, Response::Abort }, // ESP_credentials_ask_gen_overwrite
    { Response::Continue, Response::Abort }, // ESP_credentials_makefile_failed
    { Response::Continue, Response::Abort }, // ESP_credentials_load
    { Response::Retry, Response::Abort },    // ESP_credentials_invalid
    { Response::Continue },                  // ESP_credentials_uploaded
};

const PhaseResponses ClientResponses::G162Responses[CountPhases<PhasesG162>()] = {
    {}, //_first
    {}, //Parking
};
