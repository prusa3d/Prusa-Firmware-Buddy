/**
 * @file footer_item_filament.cpp
 */

#include "footer_item_filament.hpp"
#include "marlin_client.hpp"
#include "img_resources.hpp"
#include <filament.hpp>
#include <encoded_filament.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "feature/prusa/MMU2/mmu2_mk4.h"
#endif
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

FooterItemFilament::FooterItemFilament(window_t *parent)
    : FooterIconText_IntVal(parent, &img::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFilament::static_readValue() {
    const auto tool = VirtualToolIndex::currently_selected();

    if (std::holds_alternative<NoTool>(tool)) {
        return no_tool_value;
    } else {
        return EncodedFilamentType(FilamentType::for_tool(tool)).data;
    }
}

string_view_utf8 FooterItemFilament::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(no_tool_str);
    }

    static FilamentTypeParameters::Name filament;
    filament = EncodedFilamentType::from_data(value).decode().parameters().name;
    return string_view_utf8::MakeRAM(filament.data());
}
