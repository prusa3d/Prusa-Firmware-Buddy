/**
 * @file screen_menu_mmu_fail_stats.cpp
 */

#include "screen_menu_mmu_fail_stats.hpp"
#include "img_resources.hpp"

ScreenMenuMMUFailStats::ScreenMenuMMUFailStats()
    : ScreenMenuMMUFailStats__(_(label)) {
    header.SetIcon(&img::info_16x16);
}
