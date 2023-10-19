/**
 * @file print_processor.cpp
 * @author Radek Vana
 * @date 2021-05-12
 */

#include "print_processor.hpp"
#include "marlin_client.hpp"
#include "filament_sensors_handler.hpp"
#include "fsm_types.hpp"

ClientFSM type_of_q0 = ClientFSM::_none;
ClientFSM type_of_q1 = ClientFSM::_none;

void PrintProcessor::fsm_cb(uint32_t u32, uint16_t u16) {
    fsm::Change change({ u32, u16 });
    ClientFSM *fsm_type = &type_of_q0;

    // point to variable corresponding to queue from Change
    switch (change.get_queue_index()) {
    case fsm::QueueIndex::q0:
        fsm_type = &type_of_q0;
        break;
    case fsm::QueueIndex::q1:
        fsm_type = &type_of_q1;
        break;
    }

    // fsm created or destroyed
    if (*fsm_type != change.get_fsm_type()) {
        if (*fsm_type == ClientFSM::Load_unload) {
            FSensors_instance().DecEvLock(); // ClientFSM::Load_unload destroy
        } else if (change.get_fsm_type() == ClientFSM::Load_unload) {
            FSensors_instance().IncEvLock(); // ClientFSM::Load_unload create
        }
        *fsm_type = change.get_fsm_type(); // store new FSM type
    }
}

void PrintProcessor::Update() {}
void PrintProcessor::InjectGcode(const char *str) { marlin_client::gcode_push_front(str); }
bool PrintProcessor::IsPrinting() {
    return marlin_client::is_printing();
}
bool PrintProcessor::IsAutoloadEnabled() {
    return marlin_vars()->fs_autoload_enabled;
}
void PrintProcessor::Init() {
    marlin_client::set_fsm_cb(fsm_cb);
}
