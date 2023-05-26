/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * print_area.h - specify area on the bed used for printing
 */

#include <limits>
#include "../core/types.h"

class PrintArea {
  public:
    struct rect_t {
      xy_pos_t a; /// position of bottom left corner
      xy_pos_t b; /// position of upper right corner

      constexpr rect_t(float x1, float y1, float x2, float y2) : a({x1, y1}), b({x2, y2}) {}
      constexpr rect_t(xy_pos_t a, xy_pos_t b) : a(a), b(b) {}

      constexpr static rect_t max() {
        return rect_t(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
      }

      /// Check the rectangle contains given point
      bool contains(xy_pos_t point) {
        bool x_within = a.x <= point.x && point.x <= b.x;
        bool y_within = a.y <= point.y && point.y <= b.y;
        return x_within && y_within;
      }

      /// Check the rectangle contains another rect (i.e. this one is a superset of the other one)
      bool contains(rect_t other) {
        return contains(other.a) && contains(other.b);
      }

      /// Return an intersection of `this` and `other`
      rect_t intersection(rect_t other) {
        xy_pos_t ia, ib;
        ia.x = std::fmax(this->a.x, other.a.x);
        ia.y = std::fmax(this->a.y, other.a.y);
        ib.x = std::fmin(this->b.x, other.b.x);
        ib.y = std::fmin(this->b.y, other.b.y);
        return rect_t(ia, ib).normalized();
      }

      /// Return a rect with its edges moved inward by dx/dy
      rect_t inset(float dx, float dy) {
        rect_t r = *this;
        r.a.x += dx;
        r.a.y += dy;
        r.b.x -= dx;
        r.b.y -= dy;
        return r.normalized();
      }

      /// Return rect with guarantees that a.x <= b.x && a.y <= b.y
      rect_t normalized() {
        rect_t r = *this;
        if (r.b.x < r.a.x)
          r.b.x = r.a.x;
        if (r.b.y < r.a.y)
          r.b.y = r.a.y;
        return r;
      }

      /// Return area of the rectangle
      float area() const {
        return (b.x - a.x) * (b.y - a.y);
      }
    };

    /// Return current bounding rect. Returns the whole bed by default.
    static inline rect_t get_bounding_rect() { return bounding_rect; }

    /// Set current bounding rect.
    static void set_bounding_rect(rect_t bounding_rect);

    /// Set current bounding rect to max rect.
    static void reset_bounding_rect();

  private:

    /// Return the bounding rect of the bed
    static rect_t get_bed_rect();

    /// Currently set print area
    static rect_t bounding_rect;
};

extern PrintArea print_area;
