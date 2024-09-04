#pragma once

#include "eeprom_v_current.hpp"
#include <journal/backend.hpp>
#include <option/development_items.h>

/**
 * @brief Remnants of old eeprom required for migration
 *
 */
namespace config_store_ns::old_eeprom {

struct eeprom_head_t {
    uint16_t VERSION;
    uint16_t FEATURES;
    uint16_t DATASIZE;
    uint16_t FWVERSION;
    uint16_t FWBUILD;
};

// eeprom vars structure (used for defaults, packed - see above pragma)
struct eeprom_vars_t {
    eeprom_head_t head;
    old_eeprom::current::vars_body_t body;
    uint32_t CRC32;
};

inline constexpr size_t EEPROM_DATA_INIT_TRIES = 3; // maximum tries to read crc32 ok data on init
inline constexpr uint32_t EEPROM_DATASIZE = sizeof(eeprom_vars_t);
inline constexpr uint16_t EEPROM_MAX_DATASIZE = 1024; // maximum datasize
static_assert(EEPROM_DATASIZE <= EEPROM_MAX_DATASIZE, "EEPROM_MAX_DATASIZE might be outdated and not needed anymore, but EEPROM_DATASIZE shouldn't have increased anyway");

#if DEVELOPMENT_ITEMS()
    #define PRIVATE__EEPROM_OFFSET (1 << 15) // to avoid collision with public version
    #define NO_EEPROM_UPGRADES
#else
    #define PRIVATE__EEPROM_OFFSET 0
#endif

/**
 * @brief union containing eeprom struct and entire eeprom area
 * area (data) is needed for old eeprom version update and crc verification
 * because old eeprom could be bigger then current
 */
union eeprom_data {
    uint8_t data[EEPROM_MAX_DATASIZE];
    eeprom_vars_t vars;
#ifndef NO_EEPROM_UPGRADES
    struct {
        eeprom_head_t head;
        union {
            old_eeprom::v4::vars_body_t v4;
            old_eeprom::v6::vars_body_t v6;
            old_eeprom::v7::vars_body_t v7;
            old_eeprom::v9::vars_body_t v9;
            old_eeprom::v10::vars_body_t v10;
            old_eeprom::v11::vars_body_t v11;
            old_eeprom::v12::vars_body_t v12;
            old_eeprom::v32787::vars_body_t v32787;
            old_eeprom::v32789::vars_body_t v32789;
            old_eeprom::v22::vars_body_t v22;
            old_eeprom::current::vars_body_t current;
        };
    };
#endif // NO_EEPROM_UPGRADES
};

void eeprom_init_ram_mirror(eeprom_data &eeprom_ram_mirror);

bool is_older_version(const eeprom_data &eeprom_ram_mirror);

bool eeprom_convert_from(eeprom_data &data);

// version independent crc32 check
bool eeprom_check_crc32(eeprom_data &eeprom_ram_mirror);

/**
 * @brief Migrates old eeprom into new configuration store. It is done only for config_store store and only once.
 */
void migrate(old_eeprom::current::vars_body_t &body, journal::Backend &backend);
} // namespace config_store_ns::old_eeprom
