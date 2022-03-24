/**
 * @file print_processor.cpp
 * @author Radek Vana
 * @date 2021-05-12
 */

#include "print_processor.hpp"
#include "marlin_client.h"
#include "filament_sensor_api.hpp"
#include "fsm_types.hpp"
#include "non_file_printing_counter.hpp"

//TODO should I block ClientFSM::Serial_printing?
//this code did not work in last builds and no one reported problem with octoscreen
//I fear enabling it could break something
void PrintProcessor::fsm_cb(uint32_t u32, uint16_t u16) {
    fsm::variant_t variant(u32, u16);
    if (/*variant.GetType() == ClientFSM::Serial_printing ||*/ variant.GetType() == ClientFSM::Load_unload) {
        if (variant.GetCommand() == ClientFSM_Command::create) {
            FSensors_instance().IncEvLock();
        }
        if (variant.GetCommand() == ClientFSM_Command::destroy) {
            FSensors_instance().DecEvLock();
        }
    }
}

void PrintProcessor::Update() { marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_FS_AUTOLOAD_ENABLED)); }
void PrintProcessor::InjectGcode(const char *str) { marlin_gcode_push_front(str); }
bool PrintProcessor::IsPrinting() {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT));
    return marlin_vars()->sd_printing || NonFilePrintingCounter::IsPrinting();
}
bool PrintProcessor::IsAutoloadEnabled() {
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_FS_AUTOLOAD_ENABLED));
    return marlin_vars()->fs_autoload_enabled;
}
void PrintProcessor::Init() {
    marlin_client_set_fsm_cb(fsm_cb);
}
