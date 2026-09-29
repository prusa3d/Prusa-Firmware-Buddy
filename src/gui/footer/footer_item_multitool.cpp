/// @file
#include "footer_item_multitool.hpp"
#include "marlin_client.hpp"
#include <common/marlin_vars.hpp>
#include "img_resources.hpp"
#include "i18n.h"
#include <device/board.h>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

FooterItemFinda::FooterItemFinda(window_t *parent)
    : FooterIconText_IntVal(parent, &img::finda_16x16, static_makeView, static_readValue) {
}

int FooterItemFinda::static_readValue() {
    return int(marlin_vars().mmu2_finda);
}

string_view_utf8 FooterItemFinda::static_makeView(int value) {
    //@@TODO there is a strange comment in FooterItemFSensor::static_makeView about the last character not being rendered
    // Not sure why but using the same workaround.
    // Another funny thing is that the LED in FINDA shows the exact opposite - this needs to be discussed with Content ;)
    return _(value ? N_("ON ") : N_("OFF "));
}

FooterItemCurrentTool::FooterItemCurrentTool(window_t *parent)
    : FooterIconText_IntVal(parent, &img::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemCurrentTool::static_readValue() {
    return match(
        VirtualToolIndex::currently_selected(),
        [](VirtualToolIndex virtual_tool) -> int { return virtual_tool.to_raw(); },
        [](NoTool) -> int { return -1; });
}

string_view_utf8 FooterItemCurrentTool::static_makeView(int value) {
    static VirtualToolIndex::CompactDisplayName str;
    if (value >= 0) {
        str = VirtualToolIndex::from_raw(value).compact_display_name();
    } else {
        str = "-";
    }
    return string_view_utf8::MakeRAM(str.data());
}
