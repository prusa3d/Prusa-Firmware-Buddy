#pragma once

#include "guitypes.hpp"
#include <array>
#include <algorithm>
#include <numeric>
#include <limits.h> //SHRT_MAX, SHRT_MIN

////////////////////////////////////////////////////////////////////////////////
/// @enum ShiftDir_t
/// @brief A strongly typed enum class representing the direction where the
///     the next rectangle will be created
enum class ShiftDir_t {
    Left,
    Right,
    Top,
    Bottom
};

////////////////////////////////////////////////////////////////////////////////
///
/// Class represents rectangle in graphics system where x rising to the right
/// and y rising down.
///
/// @details Class also support negative value of coordinates. Such a simple perk
/// makes the implementation more robust and useful.
///
class Rect16 {
    point_i16_t top_left_;
    uint16_t width_;
    uint16_t height_;

public:
    //some structs to work with operators
    struct X_t {
        constexpr X_t(int16_t x = 0)
            : x(x) {}
        int16_t x;
        constexpr operator int16_t() const { return x; }
    };
    using Left_t = X_t;

    struct Y_t {
        constexpr Y_t(int16_t y = 0)
            : y(y) {}
        int16_t y;
        constexpr operator int16_t() const { return y; }
    };
    using Top_t = Y_t;

    struct W_t {
        constexpr W_t(uint16_t w = 0)
            : w(w) {}
        uint16_t w;
        constexpr operator uint16_t() const { return w; }
    };
    using Width_t = W_t;

    struct H_t {
        constexpr H_t(uint16_t h = 0)
            : h(h) {}
        uint16_t h;
        constexpr operator uint16_t() const { return h; }
    };
    using Height_t = H_t;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Default constructor
    /// @details set top left corner to {0,0} with width and height 0
    constexpr Rect16()
        : top_left_(point_i16_t { 0, 0 })
        , width_(0)
        , height_(0) {}

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left and bottom-right
    ///        corner
    /// @param[in] x X coordinate of top-left corner
    /// @param[in] y Y coordinate of top-left corner
    /// @param[in] x X coordinate of bottom-right corner
    /// @param[in] y Y coordinate of bottom-right corner
    //constexpr Rect16(int16_t, int16_t, int16_t, int16_t);

    constexpr Rect16(
        int16_t left,
        int16_t top,
        uint16_t width,
        uint16_t height)
        : top_left_(point_i16_t { left, top })
        , width_(width)
        , height_(height) {
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left corner and width
    ///        and height
    /// @param[in] point Top-left corner
    /// @param[in] width Width in pixels
    /// @param[in] height Height in pixels
    constexpr Rect16(point_i16_t top_left, uint16_t width, uint16_t height)
        : top_left_(top_left)
        , width_(width)
        , height_(height) {
    }

    Rect16(point_i16_t p0, point_i16_t p1);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Copy contructor as default
    constexpr Rect16(Rect16 const &) = default;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Move contructor as default
    constexpr Rect16(Rect16 &&) = default;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle as a copy of given rectangle with the shift in
    ///        specific direction
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] direction Direction where the created rectangle will be heading to
    /// @param[in] offset Offset in pixels of the new rectangle shift
    Rect16(Rect16 const &, ShiftDir_t, uint16_t);

    //position it right next this rect
    Rect16(Rect16 const &, ShiftDir_t);

    uint16_t CalculateShift(ShiftDir_t direction) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left corner and size
    ///
    /// @param[in] point Top-left corner
    /// @param[in] size Size defined by width and heigth
    Rect16(point_i16_t, size_ui16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Assign operator as default
    ///
    /// @param[in] rect Existing rectangle to duplicate into curent one
    constexpr Rect16 &operator=(Rect16 const &) & = default;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Assign operator as default
    ///
    /// @param[in] rect Existing rectangle to duplicate into curent one
    constexpr Rect16 &operator=(Rect16 &&) & = default;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the width of current rectangle
    ///
    /// @return Width of the rectangle.
    constexpr Width_t Width() const { return Width_t(width_); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the height of current rectangle
    ///
    /// @return Height of the rectangle.
    constexpr Height_t Height() const { return Height_t(height_); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the size of current rectangle
    ///
    /// @return Size of the rectangle.
    constexpr size_ui16_t Size() const { return size_ui16_t { width_, height_ }; }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the top-left corner of current rectangle
    ///
    /// @return Top-left corner of the rectangle.
    constexpr point_i16_t TopLeft() const { return top_left_; };
    constexpr point_i16_t BeginPoint() const { return TopLeft(); }; //just an alias for TopLeft()

    constexpr Top_t Top() const { return Top_t(top_left_.y); };

    constexpr Left_t Left() const { return Left_t(top_left_.x); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the bottom-right of the current rectangle
    ///
    /// @return Bottom-right of the rectangle.
    constexpr point_i16_t BottomRight() const {
        return { static_cast<int16_t>(EndPoint().x - 1), static_cast<int16_t>(EndPoint().y - 1) };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the point just behind the bottom-right of the current rectangle
    ///
    /// @return Point behind the Bottom-right of the rectangle.
    constexpr point_i16_t EndPoint() const {
        return {
            static_cast<int16_t>(top_left_.x + width_),
            static_cast<int16_t>(top_left_.y + height_)
        };
    };

    constexpr point_i16_t TopEndPoint() const {
        return {
            EndPoint().x,
            TopLeft().y
        };
    };

    constexpr point_i16_t LeftEndPoint() const {
        return {
            TopLeft().x,
            EndPoint().y
        };
    };

    //swap is not constexpr in C++17 and earlier
    void SwapXY() {
        std::swap(top_left_.x, top_left_.y);
        std::swap(width_, height_);
    }

    //mirror over X point
    constexpr void MirrorX(int16_t pt_X) { top_left_.x = static_cast<int16_t>(2 * pt_X - EndPoint().x); }
    //mirror over Y point
    constexpr void MirrorY(int16_t pt_Y) { top_left_.y = static_cast<int16_t>(2 * pt_Y - EndPoint().y); }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines if the given point is placed inside of the curent rectangle
    ///
    /// @param[in] point Point given to check
    /// @return Return true if the point is in, false otherwise.
    template <class T>
    constexpr bool Contain(point_t<T> point) const {
        return point.x >= top_left_.x
            && point.x < (top_left_.x + width_)
            && point.y >= top_left_.y
            && point.y < (top_left_.y + height_);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the intersection
    /// of two rectangles.
    ///
    /// @param[in] rect Given rectangle to check for intersection
    /// @return Return a rectangle that represents the intersection of current
    /// and given.
    Rect16 Intersection(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the union of
    /// two rectangles.
    ///
    /// @param[in] rect Given rectangle to united
    /// @return Return a rectangle that represents the union of current
    /// and given.
    Rect16 Union(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines if the given rectangle has intersection with the current
    /// one
    /// @param[in] rect Rectangle given to check
    /// @return Return true if the rectangles has intersection, false otherwise.
    bool HasIntersection(Rect16 const &) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines if the given rectangle is fully overlayed by the
    /// current on
    /// @param[in] rect Rectangle given to check
    /// @return Return true if the rectangles is subrectangle, false otherwise.
    bool Contain(Rect16 const &) const;

    void Align(Rect16 rc, uint8_t align);

    constexpr bool IsEmpty() const { return !(width_ && height_); }

    constexpr Rect16 &operator+=(X_t val) {
        top_left_.x += val.x;
        return *this;
    }
    constexpr Rect16 &operator-=(X_t val) {
        top_left_.x -= val.x;
        return *this;
    }
    constexpr Rect16 &operator=(X_t val) {
        top_left_.x = val.x;
        return *this;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, X_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, X_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    constexpr Rect16 &operator+=(Y_t val) {
        top_left_.y += val.y;
        return *this;
    }
    constexpr Rect16 &operator-=(Y_t val) {
        top_left_.y -= val.y;
        return *this;
    }
    constexpr Rect16 &operator=(Y_t val) {
        top_left_.y = val.y;
        return *this;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, Y_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, Y_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    constexpr Rect16 &operator+=(W_t val) {
        width_ += val.w;
        return *this;
    }
    constexpr Rect16 &operator-=(W_t val) {
        width_ -= val.w;
        return *this;
    }
    constexpr Rect16 &operator=(W_t val) {
        width_ = val.w;
        return *this;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, W_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, W_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    constexpr Rect16 &operator+=(H_t val) {
        height_ += val.h;
        return *this;
    }
    constexpr Rect16 &operator-=(H_t val) {
        height_ -= val.h;
        return *this;
    }
    constexpr Rect16 &operator=(H_t val) {
        height_ = val.h;
        return *this;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, H_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, H_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Add pixels to given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    /// added to origin coordinations and recent accessor call returns different
    /// value
    /// @param[in] padding Given padding structure that specify additional pixels
    template <class T>
    void AddPadding(const padding_t<T>);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Subtract pixels from given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    /// deducted from origin coordinations and recent accessor call returns
    /// different value
    /// @param[in] padding Given padding structure that specify deducted pixels
    template <class T>
    void CutPadding(const padding_t<T>);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Static method determines the rectangle structure that represents
    /// the merge of all given rectangles.
    /// @details The method is static version of Union
    /// @param[in] rectangles Collection of rectangles to merge
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    static Rect16 Merge(std::array<Rect16, SZ> const &rectangles) {
        int16_t min_x = SHRT_MAX, min_y = SHRT_MAX;
        int16_t max_x = SHRT_MIN, max_y = SHRT_MIN;

        for (size_t i = 0; i < rectangles.size(); ++i) {
            if (!rectangles[i].IsEmpty()) {
                min_x = rectangles[i].TopLeft().x < min_x ? rectangles[i].TopLeft().x : min_x;
                min_y = rectangles[i].TopLeft().y < min_y ? rectangles[i].TopLeft().y : min_y;
                max_x = rectangles[i].EndPoint().x > max_x ? rectangles[i].EndPoint().x : max_x;
                max_y = rectangles[i].EndPoint().y > max_y ? rectangles[i].EndPoint().y : max_y;
            }
        }
        if (min_x > max_x || min_y > max_y) {
            return Rect16();
        } else {
            return Rect16 { min_x, min_y, uint16_t(max_x - min_x), uint16_t(max_y - min_y) };
        }
    }

    template <class... E>
    static Rect16 Merge_ParamPack(E &&... e) {
        const size_t SZ = sizeof...(E);
        std::array<Rect16, SZ> arr = { { std::forward<E>(e)... } };
        return Merge(arr);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the union of
    /// all given rectangles.
    ///
    /// @param[in] rectangles Collection of rectangles to be united
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    Rect16 Union(std::array<Rect16, SZ> const &rectangles) {
        Rect16 ret = Rect16::Merge(rectangles);
        return Rect16 {
            TopLeft().x < ret.TopLeft().x ? TopLeft().x : ret.TopLeft().x,
            TopLeft().y < ret.TopLeft().y ? TopLeft().y : ret.TopLeft().y,
            uint16_t(EndPoint().x > ret.EndPoint().x ? EndPoint().x : ret.EndPoint().x),
            uint16_t(EndPoint().y > ret.EndPoint().y ? EndPoint().y : ret.EndPoint().y)
        };
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Split the current rectangle by given width and return such a
    /// collection of created rectangles
    /// @details The several situation should be consider
    /// 1) given width is zero => the width will be computed from the number of
    ///     records in the output collection
    /// 2) number of records * width < current rectangle width => algorithm leave
    ///     unused space of the parent rectangle
    /// 3) number of records * width > current rectangle width => algorithm fill
    ///     only records that fully fit into the parent rectangle
    /// @param[out] cuts Collection of contained sub-rectangles
    /// @param[in] width Width of the sub-rectangles
    /// @return Number of sub-rectangles
    template <size_t SZ>
    size_t HorizontalSplit(std::array<Rect16, SZ> &cuts, uint16_t width = 0) const {
        size_t i = 0;
        cuts.fill({ 0, 0, 0, 0 });
        width = (width == 0) ? Height() / SZ : width;
        while ((((i + 1) * width) < Width()) && (i < SZ)) {
            cuts[i] = {
                { static_cast<int16_t>(TopLeft().x + (i * width)), TopLeft().y },
                width,
                Height()
            };
            ++i;
        };
        return i;
    }

    /**
		 * @brief Vertical split with spaces from parent Rect16
		 * @param[out] splits[] buffer to fill of splitted Rect16
		 * @param[out] spaces[] buffer to fill of spaces between Rect16 splits
		 * @param[in] count number of splits
		 * @param[in] spacing with of spaces between rectangle's splits (optional = 0)
		 * @param[in] ratio[] ratio of wanted splits (optional = nullptr)
		 */
    void HorizontalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing = 0, uint8_t ratio[] = nullptr) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Split the current rectangle by given height and return such a
    /// collection of created rectangles
    /// @details The several situation should be consider
    /// 1) given height is zero => the height will be computed from the number of
    ///     records in the output collection
    /// 2) number of records * height < current rectangle height => algorithm leave
    ///     unused space of the parent rectangle
    /// 3) number of records * height > current rectangle height => algorithm fill
    ///     only records that fully fit into the parent rectangle
    /// @param[out] cuts Collection of contained sub-rectangles
    /// @param[in] height Height of the sub-rectangles
    /// @return Number of sub-rectangles
    template <size_t SZ>
    size_t VerticalSplit(std::array<Rect16, SZ> &cuts, uint16_t height = 0) const {
        size_t i = 0;
        cuts.fill({ 0, 0, 0, 0 });
        height = (height == 0) ? Width() / SZ : height;
        while ((((i + 1) * height) < Height()) && (i < SZ)) {
            cuts[i] = {
                { TopLeft().x, static_cast<int16_t>(TopLeft().y + (i * height)) },
                Width(),
                height
            };
            ++i;
        };
        return i;
    }

    /**
		 * @brief Vertical split with spaces from parent Rect16
		 * @param[out] splits[] buffer to fill of splitted Rect16
		 * @param[out] spaces[] buffer to fill of spaces between Rect16 splits
		 * @param[in] count number of splits
		 * @param[in] spacing with of spaces between rectangle's splits (optional = 0)
		 * @param[in] ratio[] ratio of wanted splits (optional = nullptr)
		 */
    void VerticalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing = 0, uint8_t ratio[] = nullptr) const;

    /**
     * @brief Line operation substracts subtrahend
     * Top and Height is ignored
     * @param subtrahend the rect that is to be subtracted.
     * @return Rect16 front part of original rect after substraction
     */
    Rect16 LeftSubrect(Rect16 subtrahend);

    /**
     * @brief Line operation substracts subtrahend
     * Top and Height is ignored
     * @param subtrahend the rect that is to be subtracted.
     * @return Rect16 tail part of original rect after substraction
     */
    Rect16 RightSubrect(Rect16 subtrahend);
};

////////////////////////////////////////////////////////////////////////////
/// @brief Comparison operator of the rectangle class
/// @param[in] lhs Rectangle to compare
/// @param[in] rhs Rectangle to compare
/// @return Return true when the rectangle perfectly match, false otherwise
inline bool operator==(Rect16 const &lhs, Rect16 const &rhs) {
    return lhs.TopLeft().x == rhs.TopLeft().x
        && lhs.Width() == rhs.Width()
        && lhs.Height() == rhs.Height();
}

////////////////////////////////////////////////////////////////////////////
/// @brief Not equal operator of the rectangle class
/// @param[in] lhs Rectangle to compare
/// @param[in] rhs Rectangle to compare
/// @return Return true when the rectangle not perfectly match, false otherwise
inline bool operator!=(Rect16 const &lhs, Rect16 const &rhs) {
    return !(lhs == rhs);
}

template <class T>
void Rect16::AddPadding(const padding_t<T> p) {
    top_left_.x = top_left_.x - p.left;
    top_left_.y = top_left_.y - p.top;
    width_ += (p.left + p.right);
    height_ += (p.top + p.bottom);
}

template <class T>
void Rect16::CutPadding(const padding_t<T> p) {
    if ((p.left + p.right) >= width_
        || (p.top + p.bottom) >= height_) {
        width_ = height_ = 0;
        top_left_.x = top_left_.y = 0;
    } else {
        top_left_.x = top_left_.x + p.left;
        top_left_.y = top_left_.y + p.top;
        width_ -= (p.left + p.right);
        height_ -= (p.top + p.bottom);
    }
}
