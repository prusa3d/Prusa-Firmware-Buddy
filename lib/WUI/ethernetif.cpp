#include "ethernetif.h"
#include "otp.hpp"

uint8_t *ethernetif_get_mac() {
    return const_cast<uint8_t *>(otp_get_mac_address()->mac);
}
