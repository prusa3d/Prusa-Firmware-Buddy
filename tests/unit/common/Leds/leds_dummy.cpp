#include "leds_dummy.hpp"

MK404_leds &GetLeds() {
    static MK404_leds leds;
    return leds;
}

namespace leds {

void SetNth(Color clr, index n) {
    GetLeds().leds[static_cast<size_t>(n)] = clr;
}

Color GetNth(index n) {
    return GetLeds().leds[static_cast<size_t>(n)];
}
}
