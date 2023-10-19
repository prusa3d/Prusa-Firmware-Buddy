/// @file permanent_storage.cpp
#include "permanent_storage.h"
#include "../hal/eeprom.h"
#include "globals.h"
#include "../config/config.h"
#include "axisunit.h"

#include <stddef.h>

namespace modules {
namespace permanent_storage {

#define ARR_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

/// @brief EEPROM data layout
///
/// Do not remove, reorder or change size of existing fields.
/// Otherwise values stored with previous version of firmware would be broken.
/// It is possible to add fields in the end of this struct, ensuring that erased EEPROM is handled well.
/// Last byte in EEPROM is reserved for layoutVersion. If some field is repurposed, layoutVersion
/// needs to be changed to force an EEPROM erase.
struct eeprom_t {
    uint8_t eepromLengthCorrection; ///< pre-MMU Legacy bowden length correction - not used
    uint16_t eepromBowdenLen[config::toolCount]; ///< MMU Bowden length for each filament - not used
    uint8_t eepromFilamentStatus[3]; ///< Majority vote status of eepromFilament wear leveling
    uint8_t eepromFilament[800]; ///< Top nibble status, bottom nibble last filament loaded
    uint8_t eepromDriveErrorCountH;
    uint8_t eepromDriveErrorCountL[2];
    uint8_t sg_thrs[3];
    uint16_t bowdenLengthMM; ///< MMU3 default bowden length in millimeters
} __attribute__((packed));

static_assert(sizeof(eeprom_t) - 2 <= hal::eeprom::EEPROM::End(), "eeprom_t doesn't fit into EEPROM available.");

/// @brief EEPROM layout version
static const uint8_t layoutVersion = 0xff;

//d = 6.3 mm        pulley diameter
//c = pi * d        pulley circumference
//FSPR = 200        full steps per revolution (stepper motor constant) (1.8 deg/step)
//mres = 2          pulley microstep resolution (uint8_t __res(AX_PUL))
//mres = 2          selector microstep resolution (uint8_t __res(AX_SEL))
//mres = 16         idler microstep resolution (uint8_t __res(AX_IDL))
//1 pulley ustep = (d*pi)/(mres*FSPR) = 49.48 um

// ideally, this would have been a nice constexpr (since it is a compile time constant), but the C++ standard prohibits reinterpret_casts in constexpr
static eeprom_t *const eepromBase = reinterpret_cast<eeprom_t *>(0); ///< First EEPROM address
constexpr const uint16_t eepromEmpty = 0xffffU; ///< EEPROM content when erased
constexpr const uint16_t eepromBowdenLenDefault = config::defaultBowdenLength.v; ///< Default bowden length (~360 mm)
constexpr const uint16_t eepromBowdenLenMinimum = config::minimumBowdenLength.v; ///< Minimum bowden length (~341 mm)
constexpr const uint16_t eepromBowdenLenMaximum = config::maximumBowdenLength.v; ///< Maximum bowden length (~1000 mm)

namespace ee = hal::eeprom;

#define EEOFFSET(x) reinterpret_cast<ee::EEPROM::addr_t>(&(x))

void Init() {
    if (ee::EEPROM::ReadByte(ee::EEPROM::End()) != layoutVersion) {
        EraseAll();
    }
}

void EraseAll() {
    for (uint16_t i = 0; i < ee::EEPROM::End(); i++) {
        ee::EEPROM::UpdateByte(i, static_cast<uint8_t>(eepromEmpty));
    }
    ee::EEPROM::UpdateByte(ee::EEPROM::End(), layoutVersion);
}

/// @brief Is bowden length in valid range?
/// @param BowdenLength bowden length
/// @retval true valid
/// @retval false invalid
static constexpr bool validBowdenLen(const uint16_t BowdenLength) {
    return ((BowdenLength >= eepromBowdenLenMinimum)
        && BowdenLength <= eepromBowdenLenMaximum);
}

uint16_t BowdenLength::Get() {
    uint16_t bl = ee::EEPROM::ReadWord(EEOFFSET(eepromBase->bowdenLengthMM));
    return validBowdenLen(bl) ? bl : eepromBowdenLenDefault;
}

void BowdenLength::Set(uint16_t mm) {
    ee::EEPROM::UpdateWord(EEOFFSET(eepromBase->bowdenLengthMM), mm);
}

/// @brief Get filament storage status
///
/// Uses 2 out of 3 majority vote.
///
/// @return status
/// @retval 0xff Uninitialized EEPROM or no 2 values agrees

uint8_t FilamentLoaded::getStatus() {
    if (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[0])) == ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[1])))
        return ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[0]));
    if (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[0])) == ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[2])))
        return ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[0]));
    if (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[1])) == ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[2])))
        return ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilamentStatus[1]));
    return 0xff;
}

/// @brief Set filament storage status
///
/// @retval true Succeed
/// @retval false Failed
bool FilamentLoaded::setStatus(uint8_t status) {
    for (uint8_t i = 0; i < ARR_SIZE(eeprom_t::eepromFilamentStatus); ++i) {
        ee::EEPROM::UpdateByte(EEOFFSET(eepromBase->eepromFilamentStatus[i]), status);
    }
    if (getStatus() == status)
        return true;
    return false;
}

/// @brief Get index of last valid filament
///
/// Depending on status, it searches from the beginning or from the end of eepromFilament[]
/// for the first non-matching status. Previous index (of matching status, or out of array bounds)
/// is returned.
///
/// @return index to eepromFilament[] of last valid value
/// it can be out of array range, if first item status doesn't match expected status
/// getNext(index, status) turns it to first valid index.
int16_t FilamentLoaded::getIndex() {
    const uint8_t status = getStatus();
    int16_t index = -1;
    switch (status) {
    case KeyFront1:
    case KeyFront2:
        index = ARR_SIZE(eeprom_t::eepromFilament) - 1; // It is the last one, if no dirty index found
        for (uint16_t i = 0; i < ARR_SIZE(eeprom_t::eepromFilament); ++i) {
            if (status != (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilament[i])) >> 4)) {
                index = i - 1;
                break;
            }
        }
        break;
    case KeyReverse1:
    case KeyReverse2:
        index = 0; // It is the last one, if no dirty index found
        for (int16_t i = (ARR_SIZE(eeprom_t::eepromFilament) - 1); i >= 0; --i) {
            if (status != (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilament[i])) >> 4)) {
                index = i + 1;
                break;
            }
        }
        break;
    default:
        break;
    }
    return index;
}

/// @brief Get last filament loaded
/// @param [in,out] filament filament number 0 to 4
/// @retval true success
/// @retval false failed
bool FilamentLoaded::get(uint8_t &filament) {
    int16_t index = getIndex();
    if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament)))
        return false;
    const uint8_t rawFilament = ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilament[index]));
    filament = 0x0f & rawFilament;
    if (filament >= config::toolCount)
        return false;
    const uint8_t status = getStatus();
    if (!(status == KeyFront1
            || status == KeyReverse1
            || status == KeyFront2
            || status == KeyReverse2))
        return false;
    if ((rawFilament >> 4) != status)
        return false;
    return true;
}

/// @brief Set filament being loaded
///
/// Always fails, if it is not possible to store status.
/// If it is not possible store filament, it tries all other
/// keys. Fails if storing with all other keys failed.
///
/// @param filament bottom 4 bits are stored
/// but only value 0 to 4 passes validation in FilamentLoaded::get()
/// @retval true success
/// @retval false failed
bool FilamentLoaded::set(uint8_t filament) {
    for (uint8_t i = 0; i < BehindLastKey - 1; ++i) {
        uint8_t status = getStatus();
        int16_t index = getIndex();
        getNext(status, index);
        if (!setStatus(status))
            return false;
        uint8_t filamentRaw = ((status << 4) & 0xf0) + (filament & 0x0f);
        ee::EEPROM::UpdateByte(EEOFFSET(eepromBase->eepromFilament[index]), filamentRaw);
        if (filamentRaw == ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromFilament[index])))
            return true;
        getNext(status);
        if (!setStatus(status))
            return false;
    }
    return false;
}

/// @brief Get next status and index
///
/// Get next available index following index input parameter to store filament in eepromFilament[].
/// If index would reach behind indexable space, status is updated to next and first index matching status indexing mode is returned.
/// @param [in,out] status
/// @param [in,out] index
void FilamentLoaded::getNext(uint8_t &status, int16_t &index) {
    switch (status) {
    case KeyFront1:
    case KeyFront2:
        ++index;
        if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament))) {
            getNext(status);
            index = ARR_SIZE(eeprom_t::eepromFilament) - 1;
        }
        break;
    case KeyReverse1:
    case KeyReverse2:
        --index;
        if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament))) {
            getNext(status);
            index = 0;
        }
        break;
    default:
        status = KeyFront1;
        index = 0;
        break;
    }
}

/// @brief Get next status
///
/// Sets status to next indexing mode.
///
/// @param [in,out] status
void FilamentLoaded::getNext(uint8_t &status) {
    switch (status) {
    case KeyFront1:
        status = KeyReverse1;
        break;
    case KeyReverse1:
        status = KeyFront2;
        break;
    case KeyFront2:
        status = KeyReverse2;
        break;
    case KeyReverse2:
        status = KeyFront1;
        break;
    default:
        status = KeyFront1;
        break;
    }
}

uint16_t DriveError::get() {
    return ((static_cast<uint16_t>(getH()) << 8) + getL());
}

void DriveError::increment() {
    uint16_t errors = get();
    if (errors < 0xffff) {
        ++errors;
        setL(errors);
        setH(errors >> 8);
    }
}

uint8_t DriveError::getL() {
    uint8_t first = ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromDriveErrorCountL[0]));
    uint8_t second = ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromDriveErrorCountL[1]));

    if (0xff == first && 0 == second)
        return 1;
    return (first > second) ? ++first : ++second;
}

void DriveError::setL(uint8_t lowByte) {
    ee::EEPROM::UpdateByte(EEOFFSET(eepromBase->eepromDriveErrorCountL[lowByte % 2]), lowByte - 1);
}

uint8_t DriveError::getH() {
    return (ee::EEPROM::ReadByte(EEOFFSET(eepromBase->eepromDriveErrorCountH)) + 1);
}

void DriveError::setH(uint8_t highByte) {
    ee::EEPROM::UpdateByte(EEOFFSET(eepromBase->eepromDriveErrorCountH), highByte - 1);
}

uint8_t AxisTMCSetup::get(config::Axis axis, uint8_t defaultValue) {
    return ee::EEPROM::ReadByte(EEOFFSET(eepromBase->sg_thrs[axis]), defaultValue);
}

void AxisTMCSetup::set(config::Axis axis, uint8_t val) {
    ee::EEPROM::UpdateByte(EEOFFSET(eepromBase->sg_thrs[axis]), val);
}

} // namespace permanent_storage
} // namespace modules
