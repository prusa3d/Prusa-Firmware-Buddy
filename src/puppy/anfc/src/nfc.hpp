#pragma once

#include <st25r39xxb/ST25R39XXB.hpp>
#include <prusa3d/nfc/request/config/ModulationConfig_1_0.h>

namespace nfc {
/// Constructed on first use, so that objects in other translation units can hold a reference.
st25r39xxb::ST25R39XXB &reader_1();
void readers_init();
/// For debug pruposes ONLY
void reconfigure_readers(const prusa3d_nfc_request_config_ModulationConfig_1_0 &config);
void irq();
} // namespace nfc
