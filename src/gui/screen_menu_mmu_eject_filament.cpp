/**
 * @file screen_menu_mmu_eject_filament.cpp
 */

#include "screen_menu_mmu_eject_filament.hpp"
#include "png_resources.hpp"

ScreenMenuMMUEjectFilament::ScreenMenuMMUEjectFilament()
    : ScreenMenuMMUEjectFilament__(_(label)) {
    header.SetIcon(&png::info_16x16);
}
