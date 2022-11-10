#include "printer.hpp"

#include <crc32.h>

#include <cstring>

using std::make_tuple;
using std::tuple;

namespace connect_client {

uint32_t Printer::Config::crc() const {
    uint32_t crc = 0;
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(host), strlen(host));
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(token), strlen(token));
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&port), sizeof port);
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&tls), sizeof tls);
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&enabled), sizeof enabled);
    return crc;
}

tuple<Printer::Config, bool> Printer::config(bool reset_fingerprint) {
    Config result = load_config();

    const uint32_t new_fingerprint = result.crc();
    const bool changed = new_fingerprint != cfg_fingerprint;
    if (reset_fingerprint) {
        cfg_fingerprint = new_fingerprint;
    }
    return make_tuple(result, changed);
}

}
