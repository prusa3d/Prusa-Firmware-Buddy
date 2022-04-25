#include "SteelSheets.hpp"
#include "cmath_ext.h"
#include <float.h>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <optional>

uint32_t SteelSheets::NextSheet() {
    uint8_t index = activeSheetIndex();

    for (size_t i = 1; i < eeprom_num_sheets; ++i) {
        if (IsSheetCalibrated((index + i) % eeprom_num_sheets)) {
            SelectSheet((index + i) % eeprom_num_sheets);
            return (index + i) % eeprom_num_sheets;
        }
    }
    // no calibrated sheet, set active sheet to index 0
    SelectSheet(0);
    return 0;
}
bool SteelSheets::IsSheetCalibrated(uint32_t index) {
    auto sheet = getSheet(index);
    return !nearlyEqual(sheet.z_offset, eeprom_z_offset_uncalibrated, 0.001f);
}
bool SteelSheets::SelectSheet(uint32_t index) {
    if (index >= eeprom_num_sheets)
        return false;

    uint8_t index_ui8 = index;
    eeprom_set_ui8(EEVAR_ACTIVE_SHEET, index_ui8);

    // update marlin vars
    auto sheet = getSheet(index);
    // don't need to check validity of sheet, we have checked the sheet index
    updateMarlin(sheet.z_offset);
    return true;
}
bool SteelSheets::ResetSheet(uint32_t index) {
    if (index >= eeprom_num_sheets)
        return false;
    uint8_t active = activeSheetIndex();

    setSheetOffset(index, eeprom_z_offset_uncalibrated);
    if (active == index)
        NextSheet();
    return true;
}
uint32_t SteelSheets::activeSheetIndex() {
    return eeprom_get_ui8(EEVAR_ACTIVE_SHEET);
}
uint32_t SteelSheets::NumOfCalibrated() {
    uint32_t count = 1;
    for (size_t i = 1; i < eeprom_num_sheets; ++i) {
        if (IsSheetCalibrated(i))
            ++count;
    }
    return count;
}
uint32_t SteelSheets::ActiveSheetName(char *buffer, uint32_t length) {
    if (!buffer || !length)
        return 0;
    uint8_t index = activeSheetIndex();
    return SheetName(index, buffer, length);
}
uint32_t SteelSheets::SheetName(uint32_t index, char *buffer, uint32_t length) {
    if (index >= eeprom_num_sheets || !buffer || !length)
        return 0;
    uint32_t l = length < MAX_SHEET_NAME_LENGTH ? length : MAX_SHEET_NAME_LENGTH;
    auto sheet = getSheet(index);
    memcpy(buffer, sheet.name, l);
    while (l > 0 && !buffer[l - 1])
        --l;
    return l;
}
uint32_t SteelSheets::RenameSheet(uint32_t index, const char *buffer, uint32_t length) {

    if (index >= eeprom_num_sheets || !buffer || !length)
        return false;

    auto sheet = getSheet(index);
    uint32_t l = length < MAX_SHEET_NAME_LENGTH ? length : MAX_SHEET_NAME_LENGTH;
    memset(sheet.name, 0, MAX_SHEET_NAME_LENGTH);
    memcpy(sheet.name, buffer, l);
    setSheet(index, sheet);
    return l;
}
Sheet SteelSheets::getSheet(uint32_t index) {
    return eeprom_get_sheet(index);
}
bool SteelSheets::setSheet(uint32_t index, Sheet sheet) {
    return eeprom_set_sheet(index, sheet);
}
void SteelSheets::updateMarlin(float offset) {
    offset = std::clamp(offset, zOffsetMin, zOffsetMax);
    marlin_set_var(MARLIN_VAR_Z_OFFSET, variant8_flt(offset));
    // force update marlin vars
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
}
bool SteelSheets::SetZOffset(float offset) {
    if (!std::isfinite(offset))
        offset = 0.F;
    offset = std::clamp(offset, zOffsetMin, zOffsetMax);

    uint8_t index = activeSheetIndex();
    updateMarlin(offset);
    return setSheetOffset(index, offset);
}
float SteelSheets::GetZOffset() {
    uint8_t index = activeSheetIndex();
    auto sheet = getSheet(index);
    return std::clamp(sheet.z_offset, zOffsetMin, zOffsetMax);
}

float SteelSheets::GetSheetOffset(uint32_t index) {
    auto sheet = getSheet(index);
    return std::clamp(sheet.z_offset, zOffsetMin, zOffsetMax);
}
bool SteelSheets::setSheetOffset(uint32_t index, float offset) {
    auto sheet = getSheet(index);
    sheet.z_offset = offset;
    return setSheet(index, sheet);
}
