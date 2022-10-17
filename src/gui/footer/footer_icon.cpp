/**
 * @file footer_icon.cpp
 * @author Radek Vana
 * @date 2021-03-31
 */

#include "footer_icon.hpp"
#include "guitypes.hpp"
#include "resource.h"

FooterIcon::FooterIcon(window_t *parent, png::Id id_res)
    : AddSuperWindow<window_icon_t>(
        parent,
        [parent, id_res] {
            if (!parent)
                return Rect16(); //does not have parent, cannot calculate X and Y pos

            point_i16_t pt = { 0, 0 };
            size_ui16_t sz = CalculateMinimalSize(id_res);
            size_ui16_t parent_sz = parent->GetRect().Size();

            //limit width
            sz.w = std::min(sz.w, parent_sz.w);

            //limit height and calculate virtual padding
            if (sz.h && sz.h < parent_sz.h) {
                // do not use padding to draw faster
                pt.y += (parent_sz.h - sz.h) / 2;
            } else {
                sz.h = parent_sz.h;
            }

            return Rect16(pt, sz);
        }(),
        id_res) {
}
