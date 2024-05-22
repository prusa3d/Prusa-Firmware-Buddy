#pragma once

#include "guitypes.hpp"
#include <array>
#include <algorithm>
#include <numeric>
#include <limits.h> //SHRT_MAX, SHRT_MIN
#include "align.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @enum ShiftDir_t
/// @brief A strongly typed enum class representing the direction where the
///        the next rectangle will be created
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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief private NON-template version of Rect16 Merge(std::array<Rect16, SZ> const &rectangles)
    ///        only to save some codesize, when Merge is used with different SZ
    ///        size is not checked, Merge does that
    /// @param[in] rectangles Collection of rectangles to merge
    /// @return Return a rectangle that represents the union of all rectangles
    static Rect16 merge(const Rect16 *rectangles, size_t count);

public:
    // some structs to work with operators
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
    ///
    /// @details set top left corner to {0,0} with width and height 0
    constexpr Rect16()
        : top_left_(point_i16_t { 0, 0 })
        , width_(0)
        , height_(0) {}

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left and bottom-right
    ///        corner
    /// @param[in] left X coordinate of top-left corner
    /// @param[in] top Y coordinate of top-left corner
    /// @param[in] width Width in pixels
    /// @param[in] height Height in pixels
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
    /// @param[in] top_left Top-left corner
    /// @param[in] width Width in pixels
    /// @param[in] height Height in pixels
    constexpr Rect16(point_i16_t top_left, uint16_t width, uint16_t height)
        : top_left_(top_left)
        , width_(width)
        , height_(height) {
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given 2 points
    ///        points are used as corners, usually top_left and bottom_right
    ///        bottom_left ant top_right cornres are accepted too
    /// @param[in] point first corner
    /// @param[in] point second corner
    Rect16(point_i16_t, point_i16_t);

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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle as a copy of given rectangle with the shift in
    ///        specific direction with calculated offset to be just next to it
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] direction Direction where the created rectangle will be heading to
    Rect16(Rect16 const &, ShiftDir_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given size and the shift in specific direction
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] direction Direction where the created rectangle will be heading to
    /// @param[in] offset Offset in pixels of the new rectangle shift
    /// @param[in] size Size of new rectangle
    Rect16(Rect16 const &, ShiftDir_t, size_ui16_t, uint16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given size and the shift in specific direction
    ///        with calculated offset to be next to the given rectangle
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] direction Direction where the created rectangle will be heading to
    /// @param[in] size Size of new rectangle
    Rect16(Rect16 const &, ShiftDir_t, size_ui16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given width and shifted right of the given rectangle
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] width Width of new rectangle
    /// @param[in] offset Offset in pixels of the new rectangle shift
    Rect16(Rect16 const &, Width_t, uint16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given width with calculated offset to be right
    ///        of the given rectangle
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] width Width of new rectangle
    Rect16(Rect16 const &, Width_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given height and shifted under the given rectangle
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] Height_t Height of new rectangle
    /// @param[in] offset Offset in pixels of the new rectangle shift
    Rect16(Rect16 const &, Height_t, uint16_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle with given height with calculated offset to be under
    ///        the given rectangle
    /// @param[in] rect Existing rectangle to copy
    /// @param[in] Height_t Height of new rectangle
    Rect16(Rect16 const &, Height_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Calculate offset to be able to create same rectangle in given direction
    ///
    /// @param[in] direction Direction where the created rectangle will be heading to
    uint16_t CalculateShift(ShiftDir_t) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Create rectangle on specific top-left corner and size
    ///
    /// @param[in] point Top-left corner
    /// @param[in] size Size defined by width and height
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
    /// @brief Determines the rectangle structure that represents the union of
    /// both given rectangles (smallest rectangle containing both)
    /// friends defined inside class body are inline and are hidden from non-ADL lookup
    ///
    /// @param[in] rhs rectangle to be united
    /// @return Return a rectangle that represents the union of all rectangles
    Rect16 &operator+=(Rect16 rhs);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief classic adding using += internally
    //         equal to Union
    friend inline Rect16 operator+(Rect16 lhs, Rect16 rhs) { return lhs += rhs; }

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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Alias for TopLeft
    constexpr point_i16_t BeginPoint() const { return TopLeft(); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the top coordinate
    ///        of top-left corner of current rectangle
    /// @return Top coordinate of Top-left corner of the rectangle.
    constexpr Top_t Top() const { return Top_t(top_left_.y); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the left coordinate
    ///        of top-left corner of current rectangle
    /// @return Left coordinate of Top-left corner of the rectangle.
    constexpr Left_t Left() const { return Left_t(top_left_.x); };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the bottom-right of the current rectangle
    ///
    /// @return Bottom-right of the rectangle.
    constexpr point_i16_t BottomRight() const {
        return { static_cast<int16_t>(EndPoint().x - 1), static_cast<int16_t>(EndPoint().y - 1) };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the top-right of the current rectangle
    ///
    /// @return Top-right of the rectangle.
    constexpr point_i16_t TopRight() const {
        return { BottomRight().x, Top() };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the bottom-left of the current rectangle
    ///
    /// @return Bottom-right of the rectangle.
    constexpr point_i16_t BottomLeft() const {
        return { Left(), BottomRight().y };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the bottom coordinate
    ///        of bottom-right corner of current rectangle
    /// @return Bottom coordinate of Top-left corner of the rectangle.
    constexpr Top_t Bottom() const { return Top_t(BottomRight().y); }; // Top_t return type is correct !!!

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the right coordinate
    ///        of bottom-right corner of current rectangle
    /// @return Right coordinate of Top-left corner of the rectangle.
    constexpr Left_t Right() const { return Left_t(BottomRight().x); }; // Left_t return type is correct !!!

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the point just behind the bottom-right
    ///        of the current rectangle
    /// @return Point behind the Bottom-right of the rectangle.
    constexpr point_i16_t EndPoint() const {
        return {
            static_cast<int16_t>(top_left_.x + width_),
            static_cast<int16_t>(top_left_.y + height_)
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the virtual point one pixel right of
    //         the top-right of the current rectangle
    /// @return Point right of the Top-right of the rectangle.
    constexpr point_i16_t TopEndPoint() const {
        return {
            EndPoint().x,
            TopLeft().y
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Object accessor to read the virtual point one pixel under
    //         the bottom-left of the current rectangle
    /// @return Point under the Bottom-left of the rectangle.
    constexpr point_i16_t LeftEndPoint() const {
        return {
            TopLeft().x,
            EndPoint().y
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Swap X and Y coords of current rectangle
    ///        cannot be constexpr, because swap is not constexpr in C++17
    ///        and earlier
    void SwapXY() {
        std::swap(top_left_.x, top_left_.y);
        std::swap(width_, height_);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Mirror over X point
    ///
    /// @param[in] pt_X X coorodinate of "mirror line"
    constexpr void MirrorX(int16_t pt_X) { top_left_.x = static_cast<int16_t>(2 * pt_X - EndPoint().x); }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Mirror over Y point
    ///
    /// @param[in] pt_Y Y coorodinate of "mirror line"
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
    inline Rect16 Union(Rect16 const &rc) const { return *this + rc; }

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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Align current rect with given one
    ///        changes X and Y coordinate, does not change size
    ///
    /// @param[in] rect Rectangle given to align with
    /// @param[in] align type of alignment
    void Align(Rect16, Align_t);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Transform current rect into given one (relative coords calculation)
    ///        changes X and Y coordinate and can cut size to fit
    ///        result is empty when one or both rectangle are empty
    ///
    /// @param[in] rect Rectangle given to transform into
    void Transform(Rect16 rect) {
        this->operator+=(rect.TopLeft());
        this->operator=(Intersection(rect));
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Limit size of current rect with given one
    ///        when limiting width / height / both to zero or when limiting empty rect
    ///             result just must be empty
    /// @param[in] max_sz given size limit
    constexpr void LimitSize(size_ui16_t max_sz) {
        width_ = std::min(width_, max_sz.w);
        height_ = std::min(height_, max_sz.h);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Check whether rectangle is empty
    ///
    /// @return Return true if the rectangle is empty
    constexpr bool IsEmpty() const { return !(width_ && height_); }

    // experimental features, not tested
    // TODO define meaningfull operations like X_t-X_t = W_t, X_t+W_t = X_t, X_t-W_t = X_t ...
    // TODO should not W_t be signed?
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
    constexpr void set(X_t val) {
        top_left_.x = val.x;
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
    constexpr void set(Y_t val) {
        top_left_.y = val.y;
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
    constexpr void set(W_t val) {
        width_ = val.w;
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
    constexpr void set(H_t val) {
        height_ = val.h;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, H_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, H_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    constexpr Rect16 &operator+=(point_i16_t point) {
        top_left_.x += X_t(point.x);
        top_left_.y += Y_t(point.y);
        return *this;
    }
    constexpr Rect16 &operator-=(point_i16_t point) {
        return operator+=({ int16_t(-point.x), int16_t(-point.y) });
    }
    constexpr Rect16 &operator=(point_i16_t val) {
        top_left_ = val;
        return *this;
    }
    constexpr void set(point_i16_t val) {
        top_left_ = val;
    }
    friend constexpr Rect16 operator+(Rect16 lhs, point_i16_t rhs) {
        lhs += rhs;
        return lhs;
    }
    friend constexpr Rect16 operator-(Rect16 lhs, point_i16_t rhs) {
        lhs -= rhs;
        return lhs;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Add pixels to given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    ///          added to origin coordinations and recent accessor call returns
    ///          different value
    /// @param[in] padding Given padding structure that specify additional pixels
    template <class T>
    constexpr void AddPadding(const padding_t<T>);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Add padding to given rectangle
    /// @param[in] rc Given rectangle
    /// @param[in] padding Given padding structure that specify deducted pixels
    /// @return Return rectangle containing padding
    template <class T>
    static constexpr Rect16 AddPadding(Rect16 rc, const padding_t<T> padding) {
        rc.AddPadding(padding);
        return rc;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Subtract pixels from given direction
    /// @details Such a method modify the original rectangle. The pixels will be
    ///          deducted from origin coordinations and recent accessor call
    ///          returns different value
    /// @param[in] padding Given padding structure that specify deducted pixels
    template <class T>
    constexpr void CutPadding(const padding_t<T>);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Subtract padding from given rectangle
    /// @param[in] rc Given rectangle
    /// @param[in] padding Given padding structure that specify deducted pixels
    /// @return Return cut rectangle
    template <class T>
    static constexpr Rect16 CutPadding(Rect16 rc, const padding_t<T> padding) {
        rc.CutPadding(padding);
        return rc;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Static method determines the rectangle structure that represents
    ///        the merge of all given rectangles.
    /// @details The method is static version of Union
    /// @param[in] rectangles Collection of rectangles to merge
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    static Rect16 Merge(std::array<Rect16, SZ> const &rectangles) {
        static_assert(SZ > 0, "Cannot make array with size 0");
        return merge(rectangles.begin(), SZ);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Merge version accepting parameter pack instead an array
    ///
    /// @param[in] rectangles passed by parameter pack
    /// @return Return a rectangle that represents the union of all rectangles
    template <class... E>
    static Rect16 Merge_ParamPack(E &&...e) {
        const size_t SZ = sizeof...(E);
        std::array<Rect16, SZ> arr = { { std::forward<E>(e)... } };
        return Merge(arr);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Determines the rectangle structure that represents the union of
    ///        all given rectangles.
    /// @param[in] rectangles Collection of rectangles to be united
    /// @return Return a rectangle that represents the union of all rectangles
    template <size_t SZ>
    Rect16 Union(std::array<Rect16, SZ> const &rectangles) {
        Rect16 ret = Rect16::Merge(rectangles);
        return Union(ret);
    }

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Split the current rectangle by given width and return such a
    ///        collection of created rectangles
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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Horizontal split with spaces from parent Rect16
    /// @param[out] splits[] buffer to fill of splitted Rect16
    /// @param[out] spaces[] buffer to fill of spaces between Rect16 splits
    /// @param[in] count number of splits
    /// @param[in] spacing width of spaces between rectangle's splits (optional = 0)
    /// @param[in] text_width[] width of texts (optional = nullptr)
    void HorizontalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing = 0, const uint8_t text_width[] = nullptr) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Horizontal split with dynamic spaces from parent Rect16
    ///   if splits would not fit, can decrease count (even to zero!!!)
    /// @param[out] splits[] buffer to fill of splitted Rect16
    /// @param[in] widths[] widths of wanted splits (optional = nullptr)
    /// @param[in] count number of splits
    /// @return Number of valid splits usually == count, but can be anything between 0 and count
    size_t HorizontalSplit(Rect16 splits[], Width_t widths[], size_t count) const;

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

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Vertical split with spaces from parent Rect16
    /// @param[out] splits[] buffer to fill of splitted Rect16
    /// @param[out] spaces[] buffer to fill of spaces between Rect16 splits
    /// @param[in] count number of splits
    /// @param[in] spacing with of spaces between rectangle's splits (optional = 0)
    /// @param[in] text_width[] text_width of wanted splits (optional = nullptr)
    void VerticalSplit(Rect16 splits[], Rect16 spaces[], const size_t count, const uint16_t spacing = 0, const uint8_t text_width[] = nullptr) const;

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Line operation substracts subtrahend
    /// Top and Height is ignored
    /// @param subtrahend the rect that is to be subtracted.
    /// @return Rect16 front part of original rect after substraction
    Rect16 LeftSubrect(Rect16 subtrahend);

    ////////////////////////////////////////////////////////////////////////////
    /// @brief Line operation substracts subtrahend
    /// Top and Height is ignored
    /// @param subtrahend the rect that is to be subtracted.
    /// @return Rect16 tail part of original rect after substraction
    Rect16 RightSubrect(Rect16 subtrahend);

private:
    ////////////////////////////////////////////////////////////////////////////
    /// @brief horizontal split private version for internal use only. (no checks)
    /// @param[out] splits* buffer to fill of splitted Rect16
    /// @param[in] widths* widths of rectangles
    /// @param[in] count number of splits
    static void horizontalSplit(Rect16 *splits, Width_t *widths, size_t count, Width_t width_sum, Rect16 rect);
};

////////////////////////////////////////////////////////////////////////////
/// @brief Comparison operator of the rectangle class
/// @param[in] lhs Rectangle to compare
/// @param[in] rhs Rectangle to compare
/// @return Return true when the rectangle perfectly match, false otherwise
constexpr bool operator==(Rect16 const &lhs, Rect16 const &rhs) {
    if (lhs.IsEmpty() && rhs.IsEmpty()) { // empty rects are equal
        return true;
    }
    return lhs.TopLeft() == rhs.TopLeft()
        && lhs.Width() == rhs.Width()
        && lhs.Height() == rhs.Height();
}

////////////////////////////////////////////////////////////////////////////
/// @brief Not equal operator of the rectangle class
/// @param[in] lhs Rectangle to compare
/// @param[in] rhs Rectangle to compare
/// @return Return true when the rectangle not perfectly match, false otherwise
constexpr bool operator!=(Rect16 const &lhs, Rect16 const &rhs) {
    return !(lhs == rhs);
}

////////////////////////////////////////////////////////////////////////////
/// template definitions

template <class T>
void constexpr Rect16::AddPadding(const padding_t<T> p) {
    top_left_.x = top_left_.x - p.left;
    top_left_.y = top_left_.y - p.top;
    width_ += (p.left + p.right);
    height_ += (p.top + p.bottom);
}

template <class T>
void constexpr Rect16::CutPadding(const padding_t<T> p) {
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
