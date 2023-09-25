/**
 * @file footer_eeprom.cpp
 * @author Radek Vana
 * @date 2021-05-20
 */

#include "footer_eeprom.hpp"
#include <config_store/store_instance.hpp>

using namespace footer;
using namespace footer::eeprom;

/**
 * @brief load record from eeprom
 *        check validity
 *        store valid record if was invalid
 * @return record
 */
static Record load_and_validate_record() {
    Record rec = decode(config_store().footer_setting.get());
    uint32_t valid = encode(rec);
    // cannot use Store - would recursively call this function
    config_store().footer_setting.set(valid);
    return rec;
}

static Record &get_ref() {
    static Record rec = load_and_validate_record();
    return rec;
}

Record footer::eeprom::load() {
    return get_ref();
}

changed_t footer::eeprom::store(Record rec) {
    if (get_ref() == rec)
        return changed_t::no;
    config_store().footer_setting.set(encode(rec));
    get_ref() = rec;
    return changed_t::yes;
}

bool footer::eeprom::set(Item item, size_t index) {
    if (index >= count)
        return false;
    Record rec = get_ref();
    if (rec[index] == item)
        return false;
    rec[index] = item;
    store(rec);
    return true;
}

/**
 * @brief load draw configuration from eeprom
 *        check validity
 *        store valid configuration if was invalid
 * @return ItemDrawCnf
 */
static ItemDrawCnf load_and_validate_draw_cnf() {
    ItemDrawCnf cnf = ItemDrawCnf(config_store().footer_draw_type.get());
    uint32_t valid = uint32_t(cnf);
    // cannot use Set - would recursively call this function
    config_store().footer_draw_type.set(valid);
    return cnf;
}

static ItemDrawCnf &get_draw_cnf_ref() {
    static ItemDrawCnf type = load_and_validate_draw_cnf();
    return type;
}

ItemDrawCnf footer::eeprom::load_item_draw_cnf() {
    return get_draw_cnf_ref();
}

changed_t footer::eeprom::set(ItemDrawCnf cnf) {
    if (get_draw_cnf_ref() == cnf)
        return changed_t::no;
    config_store().footer_draw_type.set(static_cast<uint32_t>(cnf));
    get_draw_cnf_ref() = cnf;
    return changed_t::yes;
}

changed_t footer::eeprom::set(ItemDrawType type) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.type = type;
    return set(cnf);
}

changed_t footer::eeprom::set(draw_zero_t zero) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.zero = zero;
    return set(cnf);
}

changed_t footer::eeprom::set_center_n_and_fewer(uint8_t center_n_and_fewer) {
    ItemDrawCnf cnf = get_draw_cnf_ref();
    cnf.center_n_and_fewer = center_n_and_fewer;
    return set(cnf);
}

ItemDrawType footer::eeprom::get_item_draw_type() {
    return get_draw_cnf_ref().type;
}

draw_zero_t footer::eeprom::get_item_draw_zero() {
    return get_draw_cnf_ref().zero;
}

uint8_t footer::eeprom::get_center_n_and_fewer() {
    return get_draw_cnf_ref().center_n_and_fewer;
}
Record footer::eeprom::decode_with_size(uint32_t encoded, size_t min_bit_size) {
    uint32_t mask = (uint32_t(1) << (min_bit_size)) - 1;
    Record ret = { {} };

    for (size_t i = 0; i < count; ++i) {
        uint32_t decoded = encoded & mask;
        if (decoded >= size_t(Item::_count))
            return footer::default_items; // data corrupted, return default setting
        ret[i] = Item(decoded);
        encoded >>= min_bit_size;
    }
    return ret;
}
Record footer::eeprom::decode(uint32_t encoded) {
    uint32_t trailing_ones = (1 << count_of_trailing_ones) - 1;
    if ((encoded & trailing_ones) != trailing_ones) {
        return footer::default_items;
    }
    encoded >>= count_of_trailing_ones;
    return decode_with_size(encoded, value_bit_size);
}
uint32_t footer::eeprom::convert_from_old_eeprom(uint32_t encoded, size_t number_of_items_in_old_eeprom) {
    const size_t old_min_eeprom_item_size = floor(log2(number_of_items_in_old_eeprom)) + 1;
    auto decoded = footer::eeprom::decode_with_size(encoded, old_min_eeprom_item_size);
    return encode(decoded);
}
