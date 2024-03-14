/**
 * @file screen_menu_mmu_eject_filament.cpp
 */

#include "screen_menu_mmu_eject_filament.hpp"
#include "img_resources.hpp"

ScreenMenuMMUEjectFilament::ScreenMenuMMUEjectFilament()
    : ScreenMenuMMUEjectFilament__(_(label)) {
    header.SetIcon(&img::info_16x16);
}
