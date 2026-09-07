/***
@brief This is power panic storage implemented with external w25x flash memory.
*/
#include "power_panic_storage.hpp"
#include <common/w25x.hpp>
#include <logging/log.hpp>
#include <bsod/bsod.h>

namespace power_panic {

LOG_COMPONENT_REF(PowerPanic);

using Storage = W25xFlash;

static Storage &storage_instance() {
    return Storage::instance();
}

static constexpr uint32_t flash_offset = Storage::pp_start_address;
static constexpr uint32_t flash_size = Storage::pp_size;

struct flash_data_t {
    fixed_t fixed;
    state_t state;

    uint8_t invalid; // boolean-like variable indicating if data is invalid
};

// in-ram copy of the flash state
state_t state_buf_data;

// Set reference to state_buf_data, to point to in-ram version of the data
state_t &state_buf = state_buf_data;

static_assert(sizeof(flash_data_t) <= flash_size, "powerpanic data exceeds reserved storage space");

// Helper functions to read/write to the flash area with type checking
#define FLASH_DATA_OFF(member) (flash_offset + offsetof(flash_data_t, member))

#define FLASH_SAVE(member, src)                                                             \
    do {                                                                                    \
        static_assert(std::is_same<decltype(flash_data_t::member), decltype(src)>::value);  \
        storage_instance().program(FLASH_DATA_OFF(member), (uint8_t *)(&src), sizeof(src)); \
    } while (0)

#define FLASH_SAVE_EXPR(member, expr)  \
    ([&]() {                           \
        decltype(member) buf = (expr); \
        FLASH_SAVE(member, buf);       \
        return buf;                    \
    }())

#define FLASH_LOAD(member, dst)                                                            \
    do {                                                                                   \
        static_assert(std::is_same<decltype(flash_data_t::member), decltype(dst)>::value); \
        storage_instance().read(FLASH_DATA_OFF(member), (uint8_t *)(&dst), sizeof(dst));   \
    } while (0)

#define FLASH_LOAD_EXPR(member)             \
    ([]() {                                 \
        decltype(flash_data_t::member) buf; \
        FLASH_LOAD(member, buf);            \
        return buf;                         \
    }())

void state_t::save() {
    auto &storage = storage_instance();
    storage.program(FLASH_DATA_OFF(state), reinterpret_cast<const uint8_t *>(&state_buf), sizeof(state_t));

    // mark as valid
    uint8_t invalid_buff = false;
    FLASH_SAVE(invalid, invalid_buff);

    if (storage.fetch_error()) {
        log_error(PowerPanic, "Failed to save live data.");
    }
}

void state_t::load() {
    auto &storage = storage_instance();
    storage.read(FLASH_DATA_OFF(state), reinterpret_cast<uint8_t *>(&state_buf), sizeof(state_t));
    runtime_state.nested_fault = true;
    if (storage.fetch_error()) {
        log_error(PowerPanic, "Load failed.");
        return;
    }
}

void erase() {
    auto &storage = storage_instance();
    for (uintptr_t addr = 0; addr < flash_size; addr += Storage::block_size) {
        storage.erase_block(flash_offset + addr);
    }
    if (storage.fetch_error()) {
        log_error(PowerPanic, "Failed to erase.");
    }
}

void fixed_t::save() {
    // print area
    PrintArea::rect_t rect = print_area.get_bounding_rect();
    FLASH_SAVE(fixed.bounding_rect_a, rect.a);
    FLASH_SAVE(fixed.bounding_rect_b, rect.b);

    // mbl
    FLASH_SAVE(fixed.z_values, unified_bed_leveling::z_values);

    // optimize the write to avoid writing the entire buffer
    auto &storage = storage_instance();
    storage.program(FLASH_DATA_OFF(fixed.media_SFN_path), (uint8_t *)runtime_state.media_SFN_path,
        strlen(runtime_state.media_SFN_path) + 1);

    if (storage.fetch_error()) {
        log_error(PowerPanic, "Failed to save fixed data.");
        return;
    }
}

void fixed_t::load() {
    // print area
    PrintArea::rect_t rect = {
        FLASH_LOAD_EXPR(fixed.bounding_rect_a),
        FLASH_LOAD_EXPR(fixed.bounding_rect_b)
    };
    print_area.set_bounding_rect(rect);

    // mbl
    FLASH_LOAD(fixed.z_values, unified_bed_leveling::z_values);

    // path
    FLASH_LOAD(fixed.media_SFN_path, runtime_state.media_SFN_path);

    if (storage_instance().fetch_error()) {
        log_error(PowerPanic, "Load failed.");
        return;
    }
}

bool state_stored() {
    bool retval = !FLASH_LOAD_EXPR(invalid);
    if (storage_instance().fetch_error()) {
        log_error(PowerPanic, "Failed to get stored.");
        return false;
    }
    return retval;
}

const char *stored_media_path() {
    debug_assert(state_stored()); // caller is responsible for checking
    FLASH_LOAD(fixed.media_SFN_path, runtime_state.media_SFN_path);
    if (storage_instance().fetch_error()) {
        log_error(PowerPanic, "Failed to get media path.");
    }
    return runtime_state.media_SFN_path;
}

} // namespace power_panic
