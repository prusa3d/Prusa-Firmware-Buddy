/**
 * @file screen_menu_mmu_load_to_nozzle.cpp
 */

#include "screen_menu_mmu_load_to_nozzle.hpp"
#include "img_resources.hpp"

ScreenMenuMMULoadToNozzle::ScreenMenuMMULoadToNozzle()
    : ScreenMenuMMULoadToNozzle__(_(label)) {
    header.SetIcon(&img::info_16x16);
}
