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
    record rec = Decode(eeprom_get_var(EEVAR_FOOTER_SETTING));
    uint32_t valid = Encode(rec);
    // cannot use Store - would recursively call this function
    eeprom_set_ui32(EEVAR_FOOTER_SETTING, valid);
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
    eeprom_set_ui32(EEVAR_FOOTER_SETTING, Encode(rec));
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
    ItemDrawCnf cnf = ItemDrawCnf(eeprom_get_var(EEVAR_FOOTER_DRAW_TYPE));
    uint32_t valid = uint32_t(cnf);
    // cannot use Set - would recursively call this function
    eeprom_set_ui32(EEVAR_FOOTER_DRAW_TYPE, valid);
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
    eeprom_set_ui32(EEVAR_FOOTER_DRAW_TYPE, uint32_t(cnf));
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
