#include "../inc/MarlinConfig.h"

#include "bedlevel/bedlevel.h"

#include "print_area.h"

PrintArea::rect_t PrintArea::bounding_rect = rect_t(X_MIN_POS, Y_MIN_POS, X_MAX_POS, Y_MAX_POS);
PrintArea print_area;

void PrintArea::set_bounding_rect(rect_t new_bounding_rect) {
  new_bounding_rect = new_bounding_rect.normalized();

  bool enlarged = !bounding_rect.contains(new_bounding_rect);
  bounding_rect = new_bounding_rect;

#if HAS_LEVELING
  if (enlarged)
    reset_bed_level();
#else
  UNUSED(enlarged);
#endif
}

PrintArea::rect_t PrintArea::get_bed_rect() {
  return rect_t(X_MIN_POS, Y_MIN_POS, X_MAX_POS, Y_MAX_POS);
}
