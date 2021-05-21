/**
 * @file footer_eeprom.cpp
 * @author Radek Vana
 * @date 2021-05-20
 */

#include "footer_eeprom.hpp"
#include "eeprom.h"

using namespace footer;
using namespace footer::eeprom;

static record &get_ref() {
    static record rec = Decode(eeprom_get_var(EEVAR_FOOTER_SETTING));
    return rec;
}

record footer::eeprom::Load() {
    return get_ref();
}

void footer::eeprom::Store(record rec) {
    eeprom_set_var(EEVAR_FOOTER_SETTING, variant8_ui32(Encode(rec)));
    get_ref() = rec;
}

bool footer::eeprom::Set(items item, size_t index) {
    if (index >= count)
        return false;
    record rec = get_ref();
    rec[index] = item;
    Store(rec);
    return true;
}

static ItemDrawCnf &getDrawCnf_ref() {
    static ItemDrawCnf type = ItemDrawCnf(eeprom_get_var(EEVAR_FOOTER_DRAW_TYPE));
    return type;
}

ItemDrawCnf footer::eeprom::LoadItemDrawCnf() {
    return getDrawCnf_ref();
}

void footer::eeprom::Set(ItemDrawCnf cnf) {
    eeprom_set_var(EEVAR_FOOTER_DRAW_TYPE, variant8_ui32(uint32_t(cnf)));
    getDrawCnf_ref() = cnf;
}

void footer::eeprom::Set(ItemDrawType type) {
    ItemDrawCnf cnf = getDrawCnf_ref();
    cnf.type = type;
    Set(cnf);
}

void footer::eeprom::Set(ItemDrawZero zero) {
    ItemDrawCnf cnf = getDrawCnf_ref();
    cnf.zero = zero;
    Set(cnf);
}
