/**
 * @file screen_menu_mmu_load_filament.cpp
 */

#include "screen_menu_mmu_load_test_filament.hpp"
#include "img_resources.hpp"

ScreenMenuMMULoadTestFilament::ScreenMenuMMULoadTestFilament()
    : ScreenMenu(_(label)) {
    header.SetIcon({ &img::info_16x16 });
}
