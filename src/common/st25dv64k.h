#pragma once

#include <option/has_nfc.h>
#include <wifi_credentials.hpp>

#include <inttypes.h>
#include <optional>

void st25dv64k_init();

uint8_t st25dv64k_user_read(uint16_t address);

void st25dv64k_user_write(uint16_t address, uint8_t data);

void st25dv64k_user_read_bytes(uint16_t address, void *pdata, uint16_t size);

void st25dv64k_user_write_bytes(uint16_t address, const void *pdata, uint16_t size);

void st25dv64k_user_unverified_write_bytes(uint16_t address, const void *pdata, uint16_t size);

uint8_t st25dv64k_rd_cfg(uint16_t address);

void st25dv64k_wr_cfg(uint16_t address, uint8_t data);

void st25dv64k_present_pwd(uint8_t *pwd);

#if HAS_NFC()

namespace nfc {

void turn_on();
void turn_off();

bool has_activity();

std::optional<WifiCredentials> consume_data();

}; // namespace nfc

#endif
