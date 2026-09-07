/***
@brief This is power panic storage implemented using coin battery backed RAM (aka Backup SRAM).
*/

#include "power_panic_storage.hpp"
#include <logging/log.hpp>
#include <crc32.h>
#include <bsod/bsod.h>

namespace power_panic {

LOG_COMPONENT_REF(PowerPanic);

// Persistent data stored in backup SRAM
struct backup_sram_data_t {
    fixed_t fixed;
    uint32_t fixed_data_crc;

    state_t state;
    uint32_t state_data_crc;

    uint32_t magic_valid; // Once stored data is valid, will be set to MAGIC_VALID_VALUE
    static constexpr uint32_t MAGIC_VALID_VALUE = 0xFA0746DC;

    void reset() {
        fixed = {};
        fixed_data_crc = 0;
        state = {};
        state_data_crc = 0;
        magic_valid = 0;
    }
};

backup_sram_data_t backup_sram_data __attribute__((section(".backup_ram")));
state_t &state_buf = backup_sram_data.state;

struct runtime_data_t {
    bool initialized = false;
    bool valid = false;
};

static runtime_data_t runtime_data;

static void initialize() {
    if (runtime_data.initialized) {
        return;
    }
    runtime_data.initialized = true;

    __HAL_RCC_BKPSRAM_CLK_ENABLE();

    bool valid = backup_sram_data.magic_valid == backup_sram_data_t::MAGIC_VALID_VALUE;
    if (valid) {
        log_info(PowerPanic, "Found panic data");
        // check CRCs
        const bool fixed_crc_valid = crc32_calc((const uint8_t *)&backup_sram_data.fixed, sizeof(backup_sram_data.fixed));
        if (!fixed_crc_valid) {
            log_error(PowerPanic, "Fixed CRC invalid");
            valid = false;
        }
        const bool state_crc_valid = crc32_calc((const uint8_t *)&backup_sram_data.state, sizeof(backup_sram_data.state));
        if (!state_crc_valid) {
            log_error(PowerPanic, "State CRC invalid");
            valid = false;
        }
    }

    if (valid) {
        log_error(PowerPanic, "Power panic data valid");
        runtime_data.valid = true;
    } else {
        // something is invalid - reset data to default
        erase();
    }

    // disable backup regulator, if another power panic happens, it will be reenabled
    HAL_PWREx_DisableBkUpReg();
}

void state_t::save() {
    // data is already stored, just add CRC and magic to mark it valid
    backup_sram_data.state_data_crc = crc32_calc((const uint8_t *)&backup_sram_data.state, sizeof(backup_sram_data.state));
    backup_sram_data.magic_valid = backup_sram_data_t::MAGIC_VALID_VALUE;

    // turn on power regulator to retain backup SRAM after power off
    HAL_PWREx_EnableBkUpReg();
}

void state_t::load() {
    // should be called after init - data should be initialized and valid
    debug_assert(runtime_data.initialized && runtime_data.valid);
}

void erase() {
    runtime_data.valid = false;
    backup_sram_data.magic_valid = 0;

    backup_sram_data.fixed_data_crc = 0;
    backup_sram_data.fixed = {};

    // state buffer might contain some data, so it is intentionally not reset, just made invalid
    // backup_sram_data.state = {};
    backup_sram_data.state_data_crc = 0;
}

void fixed_t::save() {
    // print area
    PrintArea::rect_t rect = print_area.get_bounding_rect();
    backup_sram_data.fixed.bounding_rect_a = rect.a;
    backup_sram_data.fixed.bounding_rect_b = rect.b;

    // mbl
    for (size_t x = 0; x < GRID_MAX_POINTS_X; x++) {
        for (size_t y = 0; y < GRID_MAX_POINTS_Y; y++) {
            backup_sram_data.fixed.z_values[x][y] = unified_bed_leveling::z_values[x][y];
        }
    }

    // printed file name
    memcpy(backup_sram_data.fixed.media_SFN_path, runtime_state.media_SFN_path, sizeof(backup_sram_data.fixed.media_SFN_path));

    // calculate CRC to mark data as valid
    backup_sram_data.fixed_data_crc = crc32_calc((const uint8_t *)&backup_sram_data.fixed, sizeof(backup_sram_data.fixed));
}

void fixed_t::load() {
    debug_assert(runtime_data.initialized && runtime_data.valid);

    // print area
    PrintArea::rect_t rect = {
        backup_sram_data.fixed.bounding_rect_a,
        backup_sram_data.fixed.bounding_rect_b,
    };
    print_area.set_bounding_rect(rect);

    // mbl
    for (size_t x = 0; x < GRID_MAX_POINTS_X; x++) {
        for (size_t y = 0; y < GRID_MAX_POINTS_Y; y++) {
            unified_bed_leveling::z_values[x][y] = backup_sram_data.fixed.z_values[x][y];
        }
    }

    // path
    memcpy(runtime_state.media_SFN_path, backup_sram_data.fixed.media_SFN_path, sizeof(backup_sram_data.fixed.media_SFN_path));
}

bool state_stored() {
    initialize();

    return runtime_data.valid;
}

const char *stored_media_path() {
    initialize();
    if (!runtime_data.valid) {
        return nullptr;
    }

    memcpy(runtime_state.media_SFN_path, backup_sram_data.fixed.media_SFN_path, sizeof(backup_sram_data.fixed.media_SFN_path));
    return runtime_state.media_SFN_path;
}

} // namespace power_panic
