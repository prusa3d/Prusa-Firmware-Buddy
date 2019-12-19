#include "Marlin_queue_wrapper.h"
#include "../Marlin/src/gcode/queue.h"

uint8_t get_gcode_queue_length(void) {
    return queue.length;
}
