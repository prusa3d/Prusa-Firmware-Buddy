/// @file
#include "screen_menu_mmu_preload_to_mmu.hpp"
#include "img_resources.hpp"

ScreenMenuMMUPreloadToMMU::ScreenMenuMMUPreloadToMMU()
    : ScreenMenu(_(label)) {
    header.SetIcon({ &img::info_16x16 });
}
