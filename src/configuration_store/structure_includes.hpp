/**
 * This file serves as place to add defines or includes for configuration store structure
 */
#pragma once
#include <utility>
#include <limits>
#ifndef EEPROM_UNITTEST

    #include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"
    #include "config_features.h"
#endif
#include "mem_config_item.hpp"
#ifndef EEPROM_UNITTEST
    #include "option/has_selftest.h"
    #include "footer_eeprom.hpp"

    #include "../../lib/Marlin/Marlin/src/module/temperature.h"

static constexpr float default_axis_steps_flt[4] = DEFAULT_AXIS_STEPS_PER_UNIT;
static constexpr int crash_sens[2] =
    #if ENABLED(CRASH_RECOVERY)
    CRASH_STALL_GUARD;
    #else
    { 0, 0 };
    #endif // ENABLED(CRASH_RECOVERY)

static constexpr int crash_period[2] =
    #if ENABLED(CRASH_RECOVERY)
    CRASH_PERIOD;
    #else
    { 0, 0 };
    #endif // ENABLED(CRASH_RECOVERY)

static constexpr bool crash_filter_def_val =
    #if ENABLED(CRASH_RECOVERY)
    CRASH_FILTER;
    #else
    false;
    #endif // ENABLED(CRASH_RECOVERY)

#endif

class SteelSheets {
public:
    static constexpr size_t MAX_SHEET_NAME_LENGTH = 8;
    static constexpr size_t SHEET_COUNT = 8;
    static constexpr float Z_OFFSET_UNCALIBRATED = std::numeric_limits<float>::max();
    static constexpr float Z_OFFSET_MIN = -2.0F;
    static constexpr float Z_OFFSET_MAX = 2.0F;

private:
    struct Sheet {
        std::array<char, MAX_SHEET_NAME_LENGTH> name;
        float offset;

        Sheet() = default;

        constexpr Sheet(const char *name_char, float offset)
            : offset(offset) {
            strncpy(name.data(), name_char, MAX_SHEET_NAME_LENGTH);
        }

        Sheet(const Sheet &other)
            : name(other.name)
            , offset(other.offset) {}

        Sheet(const std::array<char, MAX_SHEET_NAME_LENGTH> &name, float offset)
            : name(name)
            , offset(offset) {}

        template <class Packer>
        void pack(Packer &pack) {
            pack(name, offset);
        }

        bool operator==(const Sheet &rhs) const {
            return strcmp(name.data(), rhs.name.data()) == 0 && offset == rhs.offset;
        }

        bool operator!=(const Sheet &rhs) const {
            return !(rhs == *this);
        }
    };

    std::array<Sheet, SHEET_COUNT> sheets {
        {
            Sheet("Smooth1", std::numeric_limits<float>::max()),
            Sheet("Smooth2", std::numeric_limits<float>::max()),
            Sheet("Textur1", std::numeric_limits<float>::max()),
            Sheet("Textur2", std::numeric_limits<float>::max()),
            Sheet("Custom1", std::numeric_limits<float>::max()),
            Sheet("Custom2", std::numeric_limits<float>::max()),
            Sheet("Custom3", std::numeric_limits<float>::max()),
            Sheet("Smooth4", std::numeric_limits<float>::max()),
        },
    };
    uint8_t current_sheet = 0;
    std::optional<Sheet> get_sheet(size_t index);
    bool set_sheet(size_t index, Sheet sheet);
    void update_marlin(float offset);
    void store_changes();

public:
    SteelSheets() = default;

    SteelSheets(const SteelSheets &other)
        : sheets(other.sheets)
        , current_sheet(other.current_sheet) {
    }

    SteelSheets(std::array<Sheet, SHEET_COUNT> sheets, uint8_t currentSheet)
        : sheets(std::move(sheets))
        , current_sheet(currentSheet) {}
    bool operator==(const SteelSheets &rhs) const {
        return sheets == rhs.sheets;
    }
    size_t next_sheet();
    bool is_sheet_calibrated(size_t index);
    bool select_sheet(size_t index);
    bool reset_sheet(size_t index);
    size_t num_of_calibrated();
    std::array<char, MAX_SHEET_NAME_LENGTH> get_active_sheet_name();
    std::array<char, MAX_SHEET_NAME_LENGTH> get_sheet_name(size_t index);
    bool rename_sheet(const std::array<char, MAX_SHEET_NAME_LENGTH> &name, size_t index);
    bool set_z_offset(float offset);
    float get_z_offset();
    float get_sheet_offset(size_t index);
    bool set_sheet_offset(size_t index, float offset);
    size_t get_active_sheet_index() const;

    bool operator!=(const SteelSheets &rhs) const {
        return !(rhs == *this);
    }

    template <class Packer>
    void pack(Packer &pack) {
        pack(sheets, current_sheet);
    }
};

static_assert(sizeof(SteelSheets) < 255);
