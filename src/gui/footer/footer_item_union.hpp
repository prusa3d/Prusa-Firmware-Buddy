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
#include "footer_def.hpp"
#include "footer_item_axis.hpp"
#include "footer_item_fans.hpp"

namespace footer {
using ItemUnion = std::aligned_union<0, FooterItemNozzle, FooterItemBed, FooterItemFilament, FooterItemSpeed,
    FooterItemAxisX, FooterItemAxisY, FooterItemAxisZ, FooterItemZHeigth,
    FooterItemPrintFan, FooterItemHeatBreakFan
#if defined(FOOTER_HAS_LIVE_Z)
    ,
    FooterItemLiveZ
#endif
#if defined(FOOTER_HAS_SHEETS)
    ,
    FooterItemSheets
#endif
    >::type;

constexpr void *EncodeItemForEvent(items item) {
    return reinterpret_cast<void *>(static_cast<int>(item));
}

constexpr items DecodeItemFromEvent(void *encoded) {
    return static_cast<items>(reinterpret_cast<int>(encoded));
}

}
