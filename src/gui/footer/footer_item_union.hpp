/**
 * @file footer_item_union.hpp
 * @author Radek Vana
 * @brief union of all heater items
 * @date 2021-04-14
 */

#pragma once
#include <type_traits>
#include "footer_items_nozzle_bed.hpp"
#include "footer_item_filament.hpp"
#include "footer_item_printspeed.hpp"
#include "footer_item_live_z.hpp"
#include "footer_item_sheet_profile.hpp"

namespace footer {
using ItemUnion = std::aligned_union<0, FooterItemNozzle, FooterItemBed, FooterItemFilament,
    FooterItemSpeed, FooterItemLiveZ, FooterItemSheets>::type;

enum class items {
    ItemNozzle,
    ItemBed,
    ItemFilament,
    ItemSpeed,
    ItemLiveZ,
    ItemSheets,
    count_
};

}
