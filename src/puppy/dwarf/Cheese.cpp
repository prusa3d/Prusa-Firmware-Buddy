#include "Cheese.hpp"
#include "log.h"

LOG_COMPONENT_DEF(Cheese, LOG_SEVERITY_INFO);

bool Cheese::picked = false;
bool Cheese::parked = true;

void Cheese::update() {
    picked = Cheese::update_state(picked, get_raw_picked(), "picked");
    parked = Cheese::update_state(parked, get_raw_parked(), "parked");
}

bool Cheese::update_state(bool current_state, uint16_t raw_value, const char *name) {
    if (current_state && raw_value < HALL_THRESHOLD_LOW) {
        log_info(Cheese, "Hall sensor \"%s\" Off", name);
        return false;
    } else if (!current_state && raw_value > HALL_THRESHOLD_HIGH) {
        log_info(Cheese, "Hall sensor \"%s\" On", name);
        return true;
    } else {
        return current_state;
    }
}
