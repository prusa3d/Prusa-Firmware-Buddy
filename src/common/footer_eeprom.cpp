/**
 * @file footer_eeprom.cpp
 * @author Radek Vana
 * @date 2021-05-20
 */

#include "footer_eeprom.hpp"
#include <configuration_store.hpp>

using namespace footer;
using namespace footer::eeprom;

/**
 * @brief load record from eeprom
 *        check validity
 *        store valid record if was invalid
 * @return record
 */
static record loadAndValidateRecord() {
    record rec = Decode(config_store().footer_setting.get());
    uint32_t valid = Encode(rec);
    // cannot use Store - would recursively call this function
    config_store().footer_setting.set(valid);
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
    config_store().footer_setting.set(Encode(rec));
    get_ref() = rec;
    return changed_t::yes;
}

bool footer::eeprom::Set(Item item, size_t index) {
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
    ItemDrawCnf cnf = ItemDrawCnf(config_store().footer_draw_type.get());
    uint32_t valid = uint32_t(cnf);
    // cannot use Set - would recursively call this function
    config_store().footer_draw_type.set(valid);
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
    config_store().footer_draw_type.set(static_cast<uint32_t>(cnf));
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
record footer::eeprom::DecodeWithSize(uint32_t encoded, size_t min_bit_size) {
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    record ret = { {} };

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= size_t(Item::_count))
            return footer::DefaultItems; // data corrupted, return default setting
        ret[i] = Item(decoded);
        encoded >>= min_bit_size;
    }
    return ret;
}
record footer::eeprom::Decode(uint32_t encoded) {
    uint32_t trailing_ones = (1 << count_of_trailing_ones) - 1;
    if ((encoded & trailing_ones) != trailing_ones) {
        return footer::DefaultItems;
    }
    encoded >>= count_of_trailing_ones;
    return DecodeWithSize(encoded, value_bit_size);
}
uint32_t footer::eeprom::ConvertFromOldEeprom(uint32_t encoded, size_t number_of_items_in_old_eeprom) {
    const size_t old_min_eeprom_item_size = floor(log2(number_of_items_in_old_eeprom)) + 1;
    auto decoded = footer::eeprom::DecodeWithSize(encoded, old_min_eeprom_item_size);
    return Encode(decoded);
}
