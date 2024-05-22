/**
 * @file screen_menu_mmu_cut_filament.cpp
 */

#include "screen_menu_mmu_cut_filament.hpp"
#include "img_resources.hpp"

ScreenMenuMMUCutFilament::ScreenMenuMMUCutFilament()
    : ScreenMenu(_(label)) {
    header.SetIcon(&img::info_16x16);
}
