#pragma once

#include <type_traits>
#include "footer_items_nozzle_bed.hpp"
#include "footer_item_heatbreak.hpp"
#include "footer_item_filament.hpp"
#include "footer_item_fsensor.hpp"
#include "footer_item_printspeed.hpp"
#include "footer_item_live_z.hpp"
#include "footer_def.hpp"
#include "footer_item_axis.hpp"
#include "footer_item_fans.hpp"
#include "footer_item_multitool.hpp"
#include "footer_item_fsvalue.hpp"
#include "footer_item_input_shaper.hpp"
#include "footer_item_enclosure.hpp"
#include "footer_item_ready_status.hpp"
#include <option/has_mmu2.h>
#include <option/has_sheet_profiles.h>
#include <meta_utils.hpp>

#if HAS_SHEET_PROFILES()
    #include "footer_item_sheet_profile.hpp"
#endif

namespace footer {

template <typename T_, Item item_>
struct FooterItemMappingRec {
    using T = T_;
    static constexpr auto item = item_;
};

using FooterItemMappings = TypeList< //
    FooterItemMappingRec<FooterItemBed, Item::bed>,
    FooterItemMappingRec<FooterItemFilament, Item::filament>,
#if _DEBUG
    FooterItemMappingRec<FooterItemFSValue, Item::f_s_value>,
#endif
    FooterItemMappingRec<FooterItemFSensor, Item::f_sensor>,
    FooterItemMappingRec<FooterItemSpeed, Item::speed>,
    FooterItemMappingRec<FooterItemAxisX, Item::axis_x>,
    FooterItemMappingRec<FooterItemAxisY, Item::axis_y>,
    FooterItemMappingRec<FooterItemAxisZ, Item::axis_z>,
    FooterItemMappingRec<FooterItemZHeight, Item::z_height>,
    FooterItemMappingRec<FooterItemPrintFan, Item::print_fan>,
    FooterItemMappingRec<FooterItemHeatBreakFan, Item::heatbreak_fan>,
#if _DEBUG
    FooterItemMappingRec<FooterItemInputShaperX, Item::input_shaper_x>,
    FooterItemMappingRec<FooterItemInputShaperY, Item::input_shaper_y>,
#endif
#if defined(FOOTER_HAS_LIVE_Z)
    FooterItemMappingRec<FooterItemLiveZ, Item::live_z>,
#endif
#if !(PRINTER_IS_PRUSA_MINI || PRINTER_IS_PRUSA_MK3_5)
    FooterItemMappingRec<FooterItemHeatBreak, Item::heatbreak_temp>,
#endif
#if HAS_SHEET_PROFILES()
    FooterItemMappingRec<FooterItemSheets, Item::sheets>,
#endif
#if HAS_MMU2()
    FooterItemMappingRec<FooterItemFinda, Item::finda>,
#endif
#if defined(FOOTER_HAS_TOOL_NR)
    FooterItemMappingRec<FooterItemCurrentTool, Item::current_tool>,
    FooterItemMappingRec<FooterItemAllNozzles, Item::all_nozzles>,
#endif
#if HAS_SIDE_FSENSOR()
    FooterItemMappingRec<FooterItemFSensorSide, Item::f_sensor_side>,
#endif
    FooterItemMappingRec<FooterItemNozzleDiameter, Item::nozzle_diameter>,
    FooterItemMappingRec<FooterItemNozzlePWM, Item::nozzle_pwm>,
#if XL_ENCLOSURE_SUPPORT()
    FooterItemMappingRec<FooterItemEnclosure, Item::enclosure_temp>,
#endif
    FooterItemMappingRec<FooterItemReadyStatus, Item::ready>,
    FooterItemMappingRec<FooterItemNozzle, Item::nozzle>
    //
    >;

static_assert(FooterItemMappings::size == ftrstd::to_underlying(Item::_count) - 1 /* Item::none */ - disabled_items.size());

template <typename... Rec>
struct ItemVariantDef;

template <typename... Rec>
struct ItemVariantDef<TypeList<Rec...>> {
    using T = std::variant<std::monostate, typename Rec::T...>;
};

using ItemVariant = ItemVariantDef<FooterItemMappings>::T;

inline void *encode_item_for_event(Item item) {
    return reinterpret_cast<void *>(static_cast<intptr_t>(item));
}

inline Item decode_item_from_event(const void *const encoded) {
    return static_cast<Item>(reinterpret_cast<intptr_t>(encoded));
}

} // namespace footer
