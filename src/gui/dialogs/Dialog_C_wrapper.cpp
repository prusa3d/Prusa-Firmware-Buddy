#include "Dialog_C_wrapper.h"
#include "marlin_client.h"
#include "DialogHandler.hpp"

extern "C" {
void register_dialog_callbacks() {
    DialogHandler::Access(); //to create class NOW, not at first call of one of callback
    marlin_client_set_dialog_open_cb(DialogHandler::Open);
    marlin_client_set_dialog_close_cb(DialogHandler::Close);
    marlin_client_set_dialog_change_cb(DialogHandler::Change);
}
} //extern "C"
