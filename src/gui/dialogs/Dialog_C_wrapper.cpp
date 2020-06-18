#include "Dialog_C_wrapper.h"
#include "marlin_client.h"
#include "DialogHandler.hpp"

void register_dialog_callbacks() {
    DialogHandler::Access(); //to create class NOW, not at first call of one of callback
    marlin_client_set_fsm_create_cb(DialogHandler::Open);
    marlin_client_set_fsm_destroy_cb(DialogHandler::Close);
    marlin_client_set_fsm_change_cb(DialogHandler::Change);
}
