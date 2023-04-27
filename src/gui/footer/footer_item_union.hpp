/**
 * @file footer_item_union.hpp
 * @author Radek Vana
 * @brief union of all heater items
 * @date 2021-04-14
 */

#pragma once
#include <type_traits>
#include "footer_items_nozzle_bed.hpp"
#include "footer_item_heatbreak.hpp"
#include "footer_item_filament.hpp"
#include "footer_item_fsensor.hpp"
#include "footer_item_printspeed.hpp"
#include "footer_item_live_z.hpp"
#include "footer_item_sheet_profile.hpp"
#include "footer_def.hpp"
#include "footer_item_axis.hpp"
#include "footer_item_fans.hpp"
#include "footer_item_multitool.hpp"
#include "footer_item_fsvalue.hpp"

namespace footer {
using ItemUnion = std::aligned_union<
    0,
    FooterItemNozzle,
    FooterItemBed,
    FooterItemFilament,
    FooterItemFSensor,
    FooterItemFSValue,
    FooterItemSpeed,
    FooterItemAxisX,
    FooterItemAxisY,
    FooterItemAxisZ,
    FooterItemZHeight,
    FooterItemPrintFan,
    FooterItemHeatBreakFan
#if defined(FOOTER_HAS_LIVE_Z)
    ,
    FooterItemLiveZ
#endif
#if defined(FOOTER_HAS_SHEETS)
    ,
    FooterItemSheets
#endif
    ,
    FooterItemHeatBreak
#if HAS_MMU2
    ,
    FooterItemFinda
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    ,
    FooterItemCurrentTool,
    FooterItemAllNozzles
#endif
    >::type;

inline void *EncodeItemForEvent(Item item) {
    return reinterpret_cast<void *>(static_cast<intptr_t>(item));
}

inline Item DecodeItemFromEvent(const void *const encoded) {
    return static_cast<Item>(reinterpret_cast<intptr_t>(encoded));
}

}
