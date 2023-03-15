/**
 * @file screen_menu_mmu_load_to_nozzle.cpp
 */

#include "screen_menu_mmu_load_to_nozzle.hpp"
#include "png_resources.hpp"

ScreenMenuMMULoadToNozzle::ScreenMenuMMULoadToNozzle()
    : ScreenMenuMMULoadToNozzle__(_(label)) {
    header.SetIcon(&png::info_16x16);
}
