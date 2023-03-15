#include "animator.hpp"
#include "eeprom.h"
bool AnimatorBase::load_run_state() {
    return eeprom_get_bool(EEVAR_RUN_LEDS);
}
void AnimatorBase::save_run_state(bool state) {
    eeprom_set_bool(EEVAR_RUN_LEDS, state);
}
