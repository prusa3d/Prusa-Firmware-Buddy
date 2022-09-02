#include "cmath_ext.h"
#include <float.h>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <optional>
#include "configuration_store.hpp"
#include "marlin_client.h"

size_t SteelSheets::next_sheet() {
    uint8_t index = get_active_sheet_index();

    for (size_t i = 1; i < SHEET_COUNT; ++i) {
        if (is_sheet_calibrated((index + i) % SHEET_COUNT)) {
            select_sheet((index + i) % SHEET_COUNT);
            return (index + i) % SHEET_COUNT;
        }
    }
    // no calibrated sheet, set active sheet to index 0
    select_sheet(0);
    return 0;
}
bool SteelSheets::is_sheet_calibrated(size_t index) {
    auto sheet = get_sheet(index);
    if (sheet.has_value()) {
        return !nearlyEqual(sheet->offset, Z_OFFSET_UNCALIBRATED, 0.001f);
    }
    return false;
}
bool SteelSheets::select_sheet(size_t index) {
    auto sheet = get_sheet(index);
    if (!sheet.has_value()) {
        return false;
    }

    current_sheet = index;
    store_changes();

    // update marlin vars
    // don't need to check validity of sheet, we have checked the sheet index
    update_marlin(sheet->offset);
    return true;
}
bool SteelSheets::reset_sheet(size_t index) {
    if (index >= SHEET_COUNT)
        return false;

    // direct access used to have only one write to configuration store
    sheets[index].offset = Z_OFFSET_UNCALIBRATED;

    if (current_sheet == index) {
        // next sheet will persist the changes to configuration store
        next_sheet();
    } else {
        store_changes();
    }

    return true;
}
size_t SteelSheets::num_of_calibrated() {
    size_t count = 1;
    for (size_t i = 1; i < SHEET_COUNT; ++i) {
        if (is_sheet_calibrated(i))
            ++count;
    }
    return count;
}
std::array<char, SteelSheets::MAX_SHEET_NAME_LENGTH> SteelSheets::get_active_sheet_name() {
    return get_sheet_name(get_active_sheet_index());
}
std::array<char, SteelSheets::MAX_SHEET_NAME_LENGTH> SteelSheets::get_sheet_name(size_t index) {
    auto sheet = get_sheet(index);
    if (sheet.has_value()) {
        return sheet->name;
    }
    return std::array<char, SteelSheets::MAX_SHEET_NAME_LENGTH> { 0 };
}
std::optional<SteelSheets::Sheet> SteelSheets::get_sheet(size_t index) {
    if (index < SHEET_COUNT) {
        return sheets[index];
    }
    return std::nullopt;
}
bool SteelSheets::set_sheet(size_t index, SteelSheets::Sheet sheet) {
    if (index < SHEET_COUNT) {
        sheets[index] = sheet;
        store_changes();
        return true;
    }
    return false;
}
void SteelSheets::update_marlin(float offset) {
    offset = std::clamp(offset, Z_OFFSET_MIN, Z_OFFSET_MAX);
    marlin_set_var(MARLIN_VAR_Z_OFFSET, variant8_flt(offset));
    // force update marlin vars
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
}
bool SteelSheets::set_z_offset(float offset) {
    if (!std::isfinite(offset))
        offset = 0.F;
    offset = std::clamp(offset, Z_OFFSET_MIN, Z_OFFSET_MAX);

    uint8_t index = get_active_sheet_index();
    update_marlin(offset);
    config_store().steel_sheets.get().set_sheet_offset(index, offset);
    return true;
}

float SteelSheets::get_z_offset() {
    return get_sheet_offset(get_active_sheet_index());
}

float SteelSheets::get_sheet_offset(size_t index) {
    auto sheet = get_sheet(index);
    return std::clamp(sheet.has_value() ? sheet.value().offset : 0, Z_OFFSET_MIN, Z_OFFSET_MAX);
}

size_t SteelSheets::get_active_sheet_index() const {
    return current_sheet;
}
void SteelSheets::store_changes() {
    config_store().steel_sheets.set(*this);
}
bool SteelSheets::set_sheet_offset(size_t index, float offset) {
    auto sheet = get_sheet(index);
    if (sheet.has_value()) {
        sheet->offset = offset;
        return set_sheet(index, sheet.value());
    }
    return false;
}
