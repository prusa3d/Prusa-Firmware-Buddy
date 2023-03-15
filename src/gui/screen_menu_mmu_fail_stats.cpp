/**
 * @file screen_menu_mmu_fail_stats.cpp
 */

#include "screen_menu_mmu_fail_stats.hpp"
#include "png_resources.hpp"

ScreenMenuMMUFailStats::ScreenMenuMMUFailStats()
    : ScreenMenuMMUFailStats__(_(label)) {
    header.SetIcon(&png::info_16x16);
}
