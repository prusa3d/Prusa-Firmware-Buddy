#pragma once
#include "marlin_client.hpp"
#include <optional>
#include <span>

#include "common/sheet.hpp"

class SteelSheets {
public:
    static constexpr float zOffsetMin = Z_OFFSET_MIN;
    static constexpr float zOffsetMax = Z_OFFSET_MAX;

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Iterate across the profiles and switch to the next calibrated.
    ///
    /// Printer use print sheet profile on the index 0 as a default so the method
    /// in the worst case iterate across entire profiles and return index 0 when
    /// not any other profile is calibrated yet.
    /// @return Index of the next calibrated profile.
    static uint32_t NextSheet();
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Determine if the given sheet profile is calibrated.
    ///
    /// In case the index of the given print sheet profile is bigger than the
    /// MAX_SHEETS method return false.
    /// @param[in] index Index of the sheet profile
    /// @return True when the profile is calibrated, False othewise.
    static bool IsSheetCalibrated(uint32_t index);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Select the given print sheet profile as an active for the printer.
    /// @param[in] index Index of the sheet profile
    /// @retval true success
    /// @retval false index out of range
    static bool SelectSheet(uint32_t index);

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Reset the given print sheet profile to the uncalibrated state.
    ///
    /// @param[in] index Index of the sheet profile
    /// @retval true success
    /// @retval false index out of range
    static bool ResetSheet(uint32_t index);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Returns number of calibrated sheets
    ///
    /// Printer use print sheet profile on the index 0 as a default so the method
    /// return always at least 1 calibrated profile.
    /// @return Return the count of the calibrated print sheet profiles.
    static uint32_t NumOfCalibrated();
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Determine the name of the current active print sheet profile.
    /// !!! DOES NOT APPEND TRAILING '\0'
    ///
    /// @param[out] buffer Buffer to store the print sheet profile
    /// @param[in] length Size of the given buffer.
    /// @return Number of characters written to the buffer. Number will be
    ///        always less than MAX_SHEET_NAME_LENGTH
    static uint32_t ActiveSheetName(const std::span<char> &target);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Determine the name of the given print sheet profile.
    /// !!! DOES NOT APPEND TRAILING '\0'
    ///
    /// @param[in] index Index of the sheet profile
    /// @param[out] target Buffer to store the print sheet profile. The data will always be stored with a terminating null.
    /// @return Number of characters written to the buffer. Number will be
    ///        always less than MAX_SHEET_NAME_LENGTH
    static uint32_t SheetName(uint32_t index, const std::span<char> &target);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Rename the given print sheet profile.
    ///
    /// @param[in] index Index of the sheet profile
    /// @param[in] buffer New name of the print sheet profile
    /// @param[in] length Size of the given name.
    /// @return Number of characters written to the buffer. Number will be
    ///        always less than MAX_SHEET_NAME_LENGTH
    static uint32_t RenameSheet(uint32_t index, char const *buffer, uint32_t length);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Sets Z offset to currently selected sheet.
    ///
    /// Z offset is clamped between zOffsetMin and zOffsetMax and the unit is mm
    /// @param[in] offset  of the sheet
    static void SetZOffset(float offset);
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Gets offset of currently selected sheet.
    ///
    /// @return Z offset of the sheet
    static float GetZOffset();

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Get Z offset of sheet on index
    ///
    /// @param[in] index Index of the sheet profile
    /// @return Z offset of the sheet
    static float GetSheetOffset(uint32_t index);

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Gets the unclamped offset of currently selected sheet.
    ///
    /// @return Z offset of the sheet
    static float GetUnclampedZOffet();

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Get the unclamped Z offset of sheet with given index
    ///
    /// @param[in] index Index of the sheet profile
    /// @return Z offset of the sheet
    static float GetUnclampedSheetZOffet(uint32_t index);

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Checks if current sheet is configured, if not tries to switch to configured one
    ///
    /// Mostly needed during printers startup to check if the printer wasn't reset
    /// when configuring a new plate. That can cause that we endup with preselected
    /// non-configured sheet and we need to fix that.
    static void CheckIfCurrentValid();

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Determine the index of active sheet
    ///
    /// @return active sheet index
    static uint32_t GetActiveSheetIndex();

private:
    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Sets Z offset for sheet
    ///
    /// Z offset is clamped between zOffsetMin and zOffsetMax
    /// @param[in] index Index of the sheet profile
    /// @param[in]  offset of the sheet
    static void setSheetOffset(uint32_t index, float offset);

    static Sheet getSheet(uint32_t index);
    static void setSheet(uint32_t index, Sheet sheet);

    ///////////////////////////////////////////////////////////////////////////////
    /// @brief Updates probe Z offset variable in marlin
    ///
    /// @param[in] Z offset of the sheet
    static void updateMarlin(float offset);
};
