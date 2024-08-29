/**
 * @file footer_item_filament.cpp
 */

#include "footer_item_filament.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include "filament.hpp"
#include <config_store/store_instance.hpp>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "feature/prusa/MMU2/mmu2_mk4.h"
#endif
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

FooterItemFilament::FooterItemFilament(window_t *parent)
    : FooterIconText_IntVal(parent, &img::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
#if HAS_TOOLCHANGER()
    if (marlin_vars()->active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
        return no_tool_value;
    }
#endif /*HAS_TOOLCHANGER()*/
    uint8_t slot = marlin_vars()->active_extruder;
#if HAS_MMU2()
    if (marlin_vars()->mmu2_state.get() != static_cast<uint8_t>(MMU2::xState::Stopped)) {
        slot = MMU2::mmu2.get_current_tool();
    }
#endif
    return static_cast<int>(config_store().get_filament_type(slot));
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    auto filament = static_cast<filament::Type>(value);
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)filament::get_name(filament));
}
