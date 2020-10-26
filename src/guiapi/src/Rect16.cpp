#include "Rect16.h"

Rect16::Rect16(point_i16_t p0, point_i16_t p1)
    : top_left_(p0) {
    if (p1.x < top_left_.x)
        std::swap(p1.x, top_left_.x);
    if (p1.y < top_left_.y)
        std::swap(p1.y, top_left_.y);
    /// numbers are sorted so negative result is not possible
    width_ = p1.x - top_left_.x + 1;
    height_ = p1.y - top_left_.y + 1;
}

uint16_t Rect16::CalculateShift(ShiftDir_t direction) const {
    uint16_t distance;
    switch (direction) {

    case ShiftDir_t::Left:
    case ShiftDir_t::Right:
        distance = Width();
        break;
    case ShiftDir_t::Top:
    case ShiftDir_t::Bottom:
        distance = Height();
        break;
    default:
        distance = 0;
        break;
    }
    return distance;
}

Rect16::Rect16(Rect16 const &rect, ShiftDir_t direction)
    : Rect16(rect, direction, rect.CalculateShift(direction)) {
}

Rect16::Rect16(Rect16 const &rect, ShiftDir_t direction, uint16_t distance)
    : top_left_(
        [=] {
            point_i16_t top_left;
            switch (direction) {

            case ShiftDir_t::Left:
                top_left = {
                    static_cast<int16_t>(rect.TopLeft().x - distance),
                    rect.TopLeft().y
                };
                break;
            case ShiftDir_t::Right:
                top_left = {
                    static_cast<int16_t>(rect.TopLeft().x + distance),
                    rect.TopLeft().y
                };
                break;
            case ShiftDir_t::Top:
                top_left = {
                    rect.TopLeft().x,
                    static_cast<int16_t>(rect.TopLeft().y - distance)
                };
                break;
            case ShiftDir_t::Bottom:
                top_left = {
                    rect.TopLeft().x,
                    static_cast<int16_t>(rect.TopLeft().y + distance)
                };
                break;
            default:
                top_left = rect.TopLeft();
                break;
            }
            return top_left;
        }())
    , width_(rect.Width())
    , height_(rect.Height()) {
}

Rect16::Rect16(point_i16_t top_left, size_ui16_t s)
    : top_left_(top_left)
    , width_(s.w)
    , height_(s.h) {
}

Rect16 Rect16::Intersection(Rect16 const &r) const {
    point_i16_t top_left;
    point_i16_t bot_right;

    // If one Rect16 is on left side of other
    if (TopLeft().x > r.BottomRight().x
        || r.TopLeft().x > BottomRight().x)
        return Rect16();
    else {
        top_left.x = TopLeft().x > r.TopLeft().x
            ? TopLeft().x
            : r.TopLeft().x;
        bot_right.x = BottomRight().x < r.BottomRight().x
            ? BottomRight().x
            : r.BottomRight().x;
    }

    // If one Rect16 is above other
    if (TopLeft().y > r.BottomRight().y
        || r.TopLeft().y > BottomRight().y)
        return Rect16();
    else {
        top_left.y = TopLeft().y > r.TopLeft().y
            ? TopLeft().y
            : r.TopLeft().y;
        bot_right.y = BottomRight().y < r.BottomRight().y
            ? BottomRight().y
            : r.BottomRight().y;
    }
    return Rect16 { top_left, bot_right };
}

Rect16 Rect16::Union(Rect16 const &r) const {
    point_i16_t top_left;
    point_i16_t bot_right;

    top_left.x = TopLeft().x < r.TopLeft().x
        ? TopLeft().x
        : r.TopLeft().x;
    bot_right.x = BottomRight().x > r.BottomRight().x
        ? BottomRight().x
        : r.BottomRight().x;
    top_left.y = TopLeft().y < r.TopLeft().y
        ? TopLeft().y
        : r.TopLeft().y;
    bot_right.y = BottomRight().y > r.BottomRight().y
        ? BottomRight().y
        : r.BottomRight().y;

    return Rect16 { top_left, bot_right };
}

bool Rect16::HasIntersection(Rect16 const &r) const {
    if (r.IsEmpty())
        return false;
    return TopLeft().x < r.EndPoint().x
        && EndPoint().x > r.TopLeft().x
        && TopLeft().y < r.EndPoint().y
        && EndPoint().y > r.TopLeft().y;
}

bool Rect16::Contain(Rect16 const &r) const {
    if (r.IsEmpty())
        return true;
    return Contain(r.TopLeft()) && Contain(r.BottomRight());
}

void Rect16::Align(Rect16 rc, uint8_t align) {
    switch (align & ALIGN_HMASK) {
    case ALIGN_LEFT:
        top_left_.x = rc.Left();
        break;
    case ALIGN_RIGHT:
        top_left_.x = rc.Left() + rc.Width() - width_;
        break;
    case ALIGN_HCENTER:
        top_left_.x = rc.Left() + (rc.Width() - width_) / 2;
        break;
    }

    switch (align & ALIGN_VMASK) {
    case ALIGN_TOP:
        top_left_.y = rc.Top();
        break;
    case ALIGN_BOTTOM:
        top_left_.y = rc.Top() + rc.Height() - height_;
        break;
    case ALIGN_VCENTER:
        top_left_.y = rc.Top() + (rc.Height() - height_) / 2;
        break;
    }
}

void Rect16::HorizontalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing, uint8_t ratio[]) const {
    if (count == 0)
        return;
    if (count == 1) {
        splits[0] = *this;
        return;
    }

    size_t index, ratio_sum = 0;
    const uint16_t usable_width = Width() - (spacing * (count - 1));
    uint16_t width = usable_width / count;
    uint16_t final_width = 0;
    if (ratio != nullptr) {
        ratio_sum = std::accumulate(ratio, ratio + count, ratio_sum);
    }
    for (index = 0; index < count; index++) {
        if (ratio != nullptr) {
            width = usable_width * ((float)ratio[index] / (float)ratio_sum) + .5F;
        }
        const int16_t left = index == 0 ? (int16_t)Left() : splits[index - 1].EndPoint().x + spacing;
        /// rect split
        splits[index] = Rect16({ left, Top() }, width, Height());
        final_width += width;
        /// spaces split
        if (index < count - 1) {
            spaces[index] = Rect16({ splits[index].EndPoint().x, Top() }, spacing, Height());
            final_width += spacing;
        }
    }
    /// add not used pixels due to rounding to the last split width
    if (final_width < Width()) {
        splits[count - 1].width_ += Width() - final_width;
    }
}

void Rect16::VerticalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing, uint8_t ratio[]) const {
    if (count == 0)
        return;
    if (count == 1) {
        splits[0] = *this;
        return;
    }

    size_t index, ratio_sum = 0;
    const uint16_t usable_height = Height() - (spacing * (count - 1));
    uint16_t height = usable_height / count;
    uint16_t final_height = 0;
    if (ratio != nullptr) {
        ratio_sum = std::accumulate(ratio, ratio + count, ratio_sum);
    }
    for (index = 0; index < count; index++) {
        if (ratio != nullptr) {
            height = usable_height * ((float)ratio[index] / (float)ratio_sum) + .5F;
        }
        int16_t top = index == 0 ? (int16_t)Top() : splits[index - 1].BottomRight().y + spacing;
        /// rect split
        splits[index] = Rect16({ Left(), top }, Width(), height);
        final_height += height;
        /// spaces split
        if (index < count - 1) {
            spaces[index] = Rect16({ (int16_t)Left(), splits[index].BottomRight().y }, Width(), spacing);
            final_height += spacing;
        }
    }
    /// add not used pixels due to rounding to the last split width
    if (final_height < Height()) {
        splits[count - 1].height_ += Height() - final_height;
    }
}
