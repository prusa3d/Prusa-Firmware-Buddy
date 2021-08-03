/**
 * @file footer_eeprom.cpp
 * @author Radek Vana
 * @date 2021-05-20
 */

#include "footer_eeprom.hpp"
#include "eeprom.h"

using namespace footer;
using namespace footer::eeprom;

/**
 * @brief load record from eeprom
 *        check validity
 *        store valid record if was invalid
 * @return record
 */
static record loadAndValidateRecord() {
    uint32_t stored = eeprom_get_var(EEVAR_FOOTER_SETTING);
    record rec = Decode(stored);
    uint32_t valid = Encode(rec);
    if (stored != valid) {
        // cannot use Store - would recursively call this function
        eeprom_set_var(EEVAR_FOOTER_SETTING, variant8_ui32(valid));
    }
    return rec;
}

static record &get_ref() {
    static record rec = loadAndValidateRecord();
    return rec;
}

record footer::eeprom::Load() {
    return get_ref();
}

changed_t footer::eeprom::Store(record rec) {
    if (get_ref() == rec)
        return changed_t::no;
    eeprom_set_var(EEVAR_FOOTER_SETTING, variant8_ui32(Encode(rec)));
    get_ref() = rec;
    return changed_t::yes;
}

bool footer::eeprom::Set(items item, size_t index) {
    if (index >= count)
        return false;
    record rec = get_ref();
    if (rec[index] == item)
        return false;
    rec[index] = item;
    Store(rec);
    return true;
}

/**
 * @brief load draw configuration from eeprom
 *        check validity
 *        store valid configuration if was invalid
 * @return ItemDrawCnf
 */
static ItemDrawCnf loadAndValidateDrawCnf() {
    uint32_t stored = eeprom_get_var(EEVAR_FOOTER_DRAW_TYPE);
    ItemDrawCnf cnf = ItemDrawCnf(stored);
    uint32_t valid = uint32_t(cnf);
    if (stored != valid) {
        // cannot use Set - would recursively call this function
        eeprom_set_var(EEVAR_FOOTER_DRAW_TYPE, variant8_ui32(valid));
    }
    return cnf;
}

static ItemDrawCnf &getDrawCnf_ref() {
    static ItemDrawCnf type = loadAndValidateDrawCnf();
    return type;
}

ItemDrawCnf footer::eeprom::LoadItemDrawCnf() {
    return getDrawCnf_ref();
}

changed_t footer::eeprom::Set(ItemDrawCnf cnf) {
    if (getDrawCnf_ref() == cnf)
        return changed_t::no;
    eeprom_set_var(EEVAR_FOOTER_DRAW_TYPE, variant8_ui32(uint32_t(cnf)));
    getDrawCnf_ref() = cnf;
    return changed_t::yes;
}

changed_t footer::eeprom::Set(ItemDrawType type) {
    ItemDrawCnf cnf = getDrawCnf_ref();
    cnf.type = type;
    return Set(cnf);
}

changed_t footer::eeprom::Set(draw_zero_t zero) {
    ItemDrawCnf cnf = getDrawCnf_ref();
    cnf.zero = zero;
    return Set(cnf);
}

changed_t footer::eeprom::SetCenterNAndFewer(uint8_t centerNAndFewer) {
    ItemDrawCnf cnf = getDrawCnf_ref();
    cnf.centerNAndFewer = centerNAndFewer;
    return Set(cnf);
}

ItemDrawType footer::eeprom::GetItemDrawType() {
    return getDrawCnf_ref().type;
}

draw_zero_t footer::eeprom::GetItemDrawZero() {
    return getDrawCnf_ref().zero;
}

uint8_t footer::eeprom::GetCenterNAndFewer() {
    return getDrawCnf_ref().centerNAndFewer;
}
