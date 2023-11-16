#include "Rect16.h"

Rect16::Rect16(point_i16_t p0, point_i16_t p1)
    : top_left_(p0) {
    if (p1.x < top_left_.x) {
        std::swap(p1.x, top_left_.x);
    }
    if (p1.y < top_left_.y) {
        std::swap(p1.y, top_left_.y);
    }
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

Rect16::Rect16(Rect16 const &rect, ShiftDir_t direction, size_ui16_t size)
    : Rect16(rect, direction, rect.CalculateShift(direction)) {
    width_ = size.w;
    height_ = size.h;
}

Rect16::Rect16(Rect16 const &rect, Width_t width)
    : Rect16(rect, ShiftDir_t::Right, rect.CalculateShift(ShiftDir_t::Right)) {
    width_ = width;
}

Rect16::Rect16(Rect16 const &rect, Height_t height)
    : Rect16(rect, ShiftDir_t::Bottom, rect.CalculateShift(ShiftDir_t::Bottom)) {
    height_ = height;
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

Rect16::Rect16(Rect16 const &rect, ShiftDir_t direction, size_ui16_t size, uint16_t distance)
    : Rect16(rect, direction, distance) {
    width_ = size.w;
    height_ = size.h;
}

Rect16::Rect16(Rect16 const &rect, Width_t width, uint16_t distance)
    : Rect16(rect, ShiftDir_t::Right, distance + rect.Width()) {
    width_ = width;
}

Rect16::Rect16(Rect16 const &rect, Height_t height, uint16_t distance)
    : Rect16(rect, ShiftDir_t::Bottom, distance + rect.Height()) {
    height_ = height;
}

Rect16::Rect16(point_i16_t top_left, size_ui16_t s)
    : top_left_(top_left)
    , width_(s.w)
    , height_(s.h) {
}

Rect16 Rect16::Intersection(Rect16 const &r) const {
    point_i16_t top_left;
    point_i16_t bot_right;

    top_left.x = std::max(TopLeft().x, r.TopLeft().x);
    top_left.y = std::max(TopLeft().y, r.TopLeft().y);
    bot_right.x = std::min(BottomRight().x, r.BottomRight().x);
    bot_right.y = std::min(BottomRight().y, r.BottomRight().y);

    if (top_left.x > bot_right.x || top_left.y > bot_right.y) {
        return Rect16();
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
    if (IsEmpty() || r.IsEmpty()) {
        return false;
    }
    return TopLeft().x < r.EndPoint().x
        && EndPoint().x > r.TopLeft().x
        && TopLeft().y < r.EndPoint().y
        && EndPoint().y > r.TopLeft().y;
}

bool Rect16::Contain(Rect16 const &r) const {
    if (r.IsEmpty()) {
        return true;
    }
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

void Rect16::HorizontalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing, const uint8_t text_width[]) const {
    if (count == 0) {
        return;
    }
    if (count == 1) {
        splits[0] = *this;
        return;
    }

    size_t index;
    const uint16_t usable_width = Width() - (spacing * (count - 1));
    uint16_t width = usable_width / count;
    uint16_t final_width = 0;
    int space_in_btn = 0;
    if (text_width != nullptr) {
        int text_sum = std::accumulate(text_width, text_width + count, 0);
        if (text_sum > usable_width) {
            text_sum = usable_width;
        }
        /// unused space around text will be the same in all buttons
        space_in_btn = (usable_width - text_sum) / count;
    }

    for (index = 0; index < count; index++) {
        if (text_width != nullptr) {
            width = text_width[index] + space_in_btn;
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

/// TODO: this is not used, should share the code with HorizontalSplit
void Rect16::VerticalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing, const uint8_t ratio[]) const {
    if (count == 0) {
        return;
    }
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

size_t Rect16::HorizontalSplit(Rect16 splits[], Width_t widths[], size_t count) const {
    if (count == 0) {
        return 0;
    }
    if (IsEmpty()) {
        return 0; // nothing can fit in this rectangle
    }

    size_t used_count = 0;
    Width_t width_sum = Width_t(0);
    const Width_t width_max = Width();

    // calculate used width and used used_count of rectangles
    for (; used_count < count; ++used_count) {
        if (width_sum + widths[used_count] <= width_max) {
            // next rect fits
            width_sum = width_sum + widths[used_count];
        } else {
            // next rect does not fit
            break;
        }
    }

    horizontalSplit(splits, widths, used_count, width_sum, *this);
    return used_count;
}

void Rect16::horizontalSplit(Rect16 *splits, Width_t *widths, size_t count, Width_t width_sum, Rect16 rect) {
    // no checks, checks are in HorizontalSplit

    Rect16 first = rect;
    first = widths[0]; // width of first rect
    (*splits) = first;

    if (count > 1) {
        Width_t width_sum_spaces = rect.Width() - width_sum;
        Width_t width_space = width_sum_spaces / (count - 1);
        Width_t width_diff = width_space + widths[0]; // new rec will be this smaller

        // recalculate for recursive call
        rect -= width_diff; // rect is smaller
        rect += Left_t(width_diff); // and cut from left side
        width_sum = width_sum - widths[0];
        --count;
        widths++; // skip first
        splits++; // skip first

        // recursive call
        horizontalSplit(splits, widths, count, width_sum, rect);
        return;
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
