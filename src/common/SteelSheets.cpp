#include "SteelSheets.hpp"
#include "cmath_ext.h"
#include <float.h>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <optional>
#include <config_store/store_instance.hpp>

uint32_t SteelSheets::NextSheet() {
    uint8_t index = GetActiveSheetIndex();

    for (size_t i = 1; i < config_store_ns::sheets_num; ++i) {
        if (IsSheetCalibrated((index + i) % config_store_ns::sheets_num)) {
            SelectSheet((index + i) % config_store_ns::sheets_num);
            return (index + i) % config_store_ns::sheets_num;
        }
    }
    // no calibrated sheet, set active sheet to index 0
    SelectSheet(0);
    return 0;
}

bool SteelSheets::IsSheetCalibrated(uint32_t index) {
    auto sheet = getSheet(index);
    return std::isfinite(sheet.z_offset) && sheet.z_offset <= zOffsetMax && sheet.z_offset >= zOffsetMin;
}

bool SteelSheets::SelectSheet(uint32_t index) {
    if (index >= config_store_ns::sheets_num) {
        return false;
    }

    uint8_t index_ui8 = index;
    config_store().active_sheet.set(index_ui8);

    // update marlin vars
    auto sheet = getSheet(index);
    // don't need to check validity of sheet, we have checked the sheet index
    marlin_client::set_z_offset(sheet.z_offset);
    return true;
}

bool SteelSheets::ResetSheet(uint32_t index) {
    if (index >= config_store_ns::sheets_num) {
        return false;
    }
    uint8_t active = GetActiveSheetIndex();

    setSheetOffset(index, config_store_ns::z_offset_uncalibrated);
    if (active == index) {
        NextSheet();
    }
    return true;
}

uint32_t SteelSheets::GetActiveSheetIndex() {
    return config_store().active_sheet.get();
}

uint32_t SteelSheets::NumOfCalibrated() {
    uint32_t count = 1;
    for (size_t i = 1; i < config_store_ns::sheets_num; ++i) {
        if (IsSheetCalibrated(i)) {
            ++count;
        }
    }
    return count;
}

uint32_t SteelSheets::ActiveSheetName(std::span<char, SHEET_NAME_BUFFER_SIZE> target) {
    uint8_t index = GetActiveSheetIndex();
    return SheetName(index, target);
}

uint32_t SteelSheets::SheetName(uint32_t index, std::span<char, SHEET_NAME_BUFFER_SIZE> target) {
    if (index >= config_store_ns::sheets_num) {
        return 0;
    }
    auto sheet = getSheet(index);
    memcpy(target.data(), sheet.name, sizeof(sheet.name));
    // Make sure the string is null terminated
    target[target.size() - 1] = '\0';
    return strlen(target.data());
}

uint32_t SteelSheets::RenameSheet(uint32_t index, const char *buffer, uint32_t length) {

    if (index >= config_store_ns::sheets_num || !buffer || !length) {
        return false;
    }

    auto sheet = getSheet(index);
    uint32_t l = length < static_cast<uint32_t>(MAX_SHEET_NAME_LENGTH) ? length : static_cast<uint32_t>(MAX_SHEET_NAME_LENGTH);
    memset(sheet.name, 0, MAX_SHEET_NAME_LENGTH);
    memcpy(sheet.name, buffer, l);
    setSheet(index, sheet);
    return l;
}

Sheet SteelSheets::getSheet(uint32_t index) {
    return config_store().get_sheet(index);
}

void SteelSheets::setSheet(uint32_t index, Sheet sheet) {
    return config_store().set_sheet(index, sheet);
}

void SteelSheets::SetZOffset(float offset) {
    if (!std::isfinite(offset)) {
        offset = 0.F;
    }
    offset = std::clamp(offset, zOffsetMin, zOffsetMax);

    uint8_t index = GetActiveSheetIndex();
    marlin_client::set_z_offset(offset);
    setSheetOffset(index, offset);
}

float SteelSheets::GetZOffset() {
    return SteelSheets::GetSheetOffset(GetActiveSheetIndex());
}

float SteelSheets::GetSheetOffset(uint32_t index) {
    return std::clamp(SteelSheets::GetUnclampedSheetZOffet(index), zOffsetMin, zOffsetMax);
}

float SteelSheets::GetUnclampedZOffet() {
    return GetUnclampedSheetZOffet(GetActiveSheetIndex());
}

float SteelSheets::GetUnclampedSheetZOffet(uint32_t index) {
    auto sheet = getSheet(index);
    return sheet.z_offset;
}

void SteelSheets::setSheetOffset(uint32_t index, float offset) {
    auto sheet = getSheet(index);
    sheet.z_offset = offset;
    setSheet(index, sheet);
}

void SteelSheets::CheckIfCurrentValid() {
    auto sheet = getSheet(GetActiveSheetIndex());
    if (sheet.z_offset < zOffsetMin || sheet.z_offset > zOffsetMax) {
        NextSheet();
    }
}
