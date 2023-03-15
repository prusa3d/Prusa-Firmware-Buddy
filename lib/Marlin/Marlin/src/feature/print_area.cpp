#include "../inc/MarlinConfig.h"

#if ENABLED(MODULAR_HEATBED)
  #include "../module/temperature.h"
  #include "../module/modular_heatbed.h"
#endif

#include "bedlevel/bedlevel.h"

#include "print_area.h"

PrintArea::rect_t PrintArea::bounding_rect = rect_t(X_MIN_POS, Y_MIN_POS, X_MAX_POS, Y_MAX_POS);
PrintArea print_area;

void PrintArea::set_bounding_rect(rect_t new_bounding_rect) {
  new_bounding_rect = new_bounding_rect.normalized();

  bool enlarged = !bounding_rect.contains(new_bounding_rect);
  bounding_rect = new_bounding_rect;

#if ENABLED(MODULAR_HEATBED)
  uint16_t enabled_bedlet_mask = 0;
  for (int x = 0; x < X_HBL_COUNT; x++) {
    for (int y = 0; y < Y_HBL_COUNT; y++) {
      rect_t hbl_rect = rect_t(x * X_HBL_SIZE, y * Y_HBL_SIZE, (x + 1) * X_HBL_SIZE, (y + 1) * Y_HBL_SIZE);
      bool enabled = bounding_rect.intersection(hbl_rect).area() > 0;
      if (enabled)
          enabled_bedlet_mask |= 1 << advanced_modular_bed->idx(x, y);
    }
  }
  if (PRINT_AREA_BASED_HEATING_ENABLED) {
    thermalManager.setEnabledBedletMask(enabled_bedlet_mask);
  }
#endif

#if HAS_LEVELING
  if (enlarged)
    reset_bed_level();
#else
  UNUSED(enlarged);
#endif
}

void PrintArea::reset_bounding_rect() {
   set_bounding_rect(rect_t::max());
}

PrintArea::rect_t PrintArea::get_bed_rect() {
  return rect_t(X_MIN_POS, Y_MIN_POS, X_MAX_POS, Y_MAX_POS);
}
