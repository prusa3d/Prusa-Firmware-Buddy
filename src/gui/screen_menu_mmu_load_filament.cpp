/**
 * @file screen_menu_mmu_load_filament.cpp
 */

#include "screen_menu_mmu_load_filament.hpp"
#include "png_resources.hpp"

ScreenMenuMMULoadFilament::ScreenMenuMMULoadFilament()
    : ScreenMenuMMULoadFilament__(_(label)) {
    header.SetIcon({ &png::info_16x16 });
}
