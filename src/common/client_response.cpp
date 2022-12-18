#include "client_response.hpp"

//define available responses for every phase
const PhaseResponses ClientResponses::LoadUnloadResponses[CountPhases<PhasesLoadUnload>()] = {
    {},                                                       //_first
    { Response::Stop },                                       //Parking_stoppable
    {},                                                       //Parking_unstoppable,
    { Response::Stop },                                       //WaitingTemp_stoppable,
    {},                                                       //WaitingTemp_unstoppable,
    { Response::Stop },                                       //PreparingToRam_stoppable,
    {},                                                       //PreparingToRam_unstoppable
    { Response::Stop },                                       //Ramming_stoppable,
    {},                                                       //Ramming_unstoppable,
    { Response::Stop },                                       //Unloading_stoppable,
    {},                                                       //Unloading_unstoppable,
    { Response::Filament_removed },                           //RemoveFilament,
    { Response::Yes, Response::No },                          //IsFilamentUnloaded,
    {},                                                       //FilamentNotInFS
    { Response::Continue },                                   //ManualUnload,
    { Response::Continue, Response::Stop },                   //UserPush_stoppable,
    { Response::Continue },                                   //UserPush_unstoppable,
    { Response::Stop },                                       //MakeSureInserted_stoppable,
    {},                                                       //MakeSureInserted_unstoppable,
    { Response::Stop },                                       //Inserting_stoppable,
    {},                                                       //Inserting_unstoppable,
    { Response::Yes, Response::No },                          //IsFilamentInGear,
    { Response::Stop },                                       //Ejecting_stoppable,
    {},                                                       //Ejecting_unstoppable,
    { Response::Stop },                                       //Loading_stoppable,
    {},                                                       //Loading_unstoppable,
    { Response::Stop },                                       //Purging_stoppable,
    {},                                                       //Purging_unstoppable,
    { Response::Yes, Response::Purge_more, Response::Retry }, //IsColor,
    { Response::Yes, Response::Purge_more },                  //IsColorPurge
    {},                                                       //Unparking

};

const PhaseResponses ClientResponses::PreheatResponses[CountPhases<PhasesPreheat>()] = {
    {}, //_first
    { Response::Abort, Response::Cooldown, Response::PLA, Response::PETG,
        Response::ASA, Response::ABS, Response::PC, Response::FLEX, Response::HIPS, Response::PP, Response::PVB }, //UserTempSelection
};

const PhaseResponses ClientResponses::PrintPreviewResponses[CountPhases<PhasesPrintPreview>()] = {
    { Response::Print, Response::Back },                    // main_dialog,
    { Response::Abort, Response::Ignore },                  // wrong_printer
    { Response::Yes, Response::No, Response::FS_disable },  // filament_not_inserted
    { Response::Yes, Response::No },                        // mmu_filament_inserted
    { Response::Change, Response::Ignore, Response::Abort } // wrong_filament
};

const PhaseResponses ClientResponses::CrashRecoveryResponses[CountPhases<PhasesCrashRecovery>()] = {
    {},                                                     //check X == _first
    {},                                                     //check Y
    {},                                                     //home
    { Response::Retry, Response::Pause, Response::Resume }, //axis NOK
    {},                                                     //axis short
    {},                                                     //axis long
    { Response::Resume, Response::Pause },                  //repeated crash
};

const PhaseResponses ClientResponses::SelftestResponses[CountPhases<PhasesSelftest>()] = {
    {}, // _none == _first

    { Response::Continue, Response::Cancel },                   // WizardPrologue_ask_run
    { Response::Continue, Response::Cancel, Response::Ignore }, // WizardPrologue_ask_run_dev
    { Response::Continue, Response::Cancel },                   // WizardPrologue_info
    { Response::Continue, Response::Cancel },                   // WizardPrologue_info_detailed

    { Response::Continue, Response::Abort }, // ESP_instructions
    { Response::Continue, Response::Skip },  // ESP_USB_not_inserted
    { Response::Continue, Response::Skip },  // ESP_ask_gen
    { Response::Continue, Response::Skip },  // ESP_ask_gen_overwrite
    { Response::Continue, Response::Skip },  // ESP_makefile_failed
    { Response::Continue },                  // ESP_eject_USB
    { Response::Continue, Response::Abort }, // ESP_insert_USB
    { Response::Retry, Response::Abort },    // ESP_invalid
    { Response::Abort },                     // ESP_uploading_config
    { Response::Continue },                  // ESP_enabling_WIFI
    { Response::Continue },                  // ESP_uploaded

    { Response::Continue, Response::Abort }, // ESP_progress_info
    { Response::Abort },                     // ESP_progress_upload
    { Response::Continue },                  // ESP_progress_passed
    { Response::Continue },                  // ESP_progress_failed

    { Response::Continue, Response::Abort }, // ESP_qr_instructions_flash
    { Response::Continue, Response::Abort }, // ESP_qr_instructions

    {}, // Fans

    {}, // Axis

    {}, // Heaters

    {}, // FirstLayer_mbl
    {}, // FirstLayer_print

    { Response::Next, Response::Unload },                 // FirstLayer_filament_known_and_not_unsensed = _first_FirstLayerQuestions
    { Response::Next, Response::Load, Response::Unload }, // FirstLayer_filament_not_known_or_unsensed
    { Response::Next },                                   // FirstLayer_calib
    { Response::Yes, Response::No },                      // FirstLayer_use_val
    { Response::Next },                                   // FirstLayer_start_print
    { Response::Yes, Response::No },                      // FirstLayer_reprint
    { Response::Next },                                   // FirstLayer_clean_sheet
    { Response::Next },                                   // FirstLayer_failed

    { Response::Next }, // Result

    { Response::Continue }, // WizardEpilogue_ok
    { Response::Continue }, // WizardEpilogue_nok
};
