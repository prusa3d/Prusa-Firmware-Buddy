/// @file
#include "screen_menu_mmu_load_filament.hpp"
#include "img_resources.hpp"

ScreenMenuMMUPreloadToMMU::ScreenMenuMMUPreloadToMMU()
    : ScreenMenuMMUPreloadToMMU__(_(label)) {
    header.SetIcon({ &img::info_16x16 });
}
