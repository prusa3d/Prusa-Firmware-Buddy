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
    if (IsEmpty() || r.IsEmpty())
        return Rect16(0, 0, 0, 0);
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

Rect16 &Rect16::operator+=(Rect16 rhs) {
    // this is empty, rhs is not .. just replace this with rhs
    if ((this->IsEmpty()) && (!rhs.IsEmpty())) {
        *this = rhs;
    }

    if ((!this->IsEmpty()) && (!rhs.IsEmpty())) {
        int16_t max_x = std::max(EndPoint().x, rhs.EndPoint().x);
        int16_t max_y = std::max(EndPoint().y, rhs.EndPoint().y);

        top_left_.x = std::min(top_left_.x, rhs.TopLeft().x);
        top_left_.y = std::min(top_left_.y, rhs.TopLeft().y);
        width_ = uint16_t(max_x - top_left_.x);
        height_ = uint16_t(max_y - top_left_.y);
    }

    return *this;
}

bool Rect16::HasIntersection(Rect16 const &r) const {
    if (IsEmpty() || r.IsEmpty())
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

void Rect16::Align(Rect16 rc, Align_t align) {
    switch (align.Horizontal()) {
    case Align_t::horizontal::left:
        top_left_.x = rc.Left();
        break;
    case Align_t::horizontal::right:
        top_left_.x = rc.Left() + rc.Width() - width_;
        break;
    case Align_t::horizontal::center:
        top_left_.x = rc.Left() + (rc.Width() - width_) / 2;
        break;
    }

    switch (align.Vertical()) {
    case Align_t::vertical::top:
        top_left_.y = rc.Top();
        break;
    case Align_t::vertical::bottom:
        top_left_.y = rc.Top() + rc.Height() - height_;
        break;
    case Align_t::vertical::center:
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

Rect16 Rect16::LeftSubrect(Rect16 subtrahend) {
    Rect16 ret = *this;
    if (subtrahend.Left() < Left()) {
        ret = Width_t(0);
        return ret;
    }

    if (subtrahend.Left() >= (Left() + Width())) {
        return ret;
    }

    ret = Width_t(subtrahend.Left() - ret.Left());
    return ret;
}

Rect16 Rect16::RightSubrect(Rect16 subtrahend) {
    Rect16 ret = *this;

    if (subtrahend.Left() + subtrahend.Width() >= Left() + Width()) {
        ret = Width_t(0);
        return ret;
    }

    ret = Left_t(subtrahend.Left() + subtrahend.Width());
    ret -= Width_t(subtrahend.Left() - Left());
    ret -= subtrahend.Width();
    return ret;
}

Rect16 Rect16::merge(const Rect16 *rectangles, size_t count) {
    // this is private method called by public one(s)
    // public method must not set count == 0
    // public method is a template one and count is checked at compile time
    Rect16 ret = rectangles[0];

    for (size_t i = 1; i < count; ++i) {
        ret += rectangles[i];
    }
    return ret;
}
