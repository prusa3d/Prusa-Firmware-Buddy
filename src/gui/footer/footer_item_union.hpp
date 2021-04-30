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
#include "printers.h"

#if defined(PRINTER_TYPE) && PRINTER_TYPE == PRINTER_PRUSA_MINI
    #define FOOTER_HAS_LIVE_Z
    #define FOOTER_HAS_SHEETS
#endif

namespace footer {
using ItemUnion = std::aligned_union<0, FooterItemNozzle, FooterItemBed, FooterItemFilament, FooterItemSpeed
#if defined(FOOTER_HAS_LIVE_Z)
    ,
    FooterItemLiveZ
#endif
#if defined(FOOTER_HAS_SHEETS)
    ,
    FooterItemSheets
#endif
    >::type;

enum class items {
    ItemNozzle,
    ItemBed,
    ItemFilament,
    ItemSpeed,
#if defined(FOOTER_HAS_LIVE_Z)
    ItemLiveZ,
#endif
#if defined(FOOTER_HAS_SHEETS)
    ItemSheets,
#endif
    count_
};

constexpr void *EncodeItemForEvent(items item) {
    return reinterpret_cast<void *>(static_cast<int>(item));
}

constexpr items DecodeItemFromEvent(void *encoded) {
    return static_cast<items>(reinterpret_cast<int>(encoded));
}

}
