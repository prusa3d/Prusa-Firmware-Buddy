/**
 * @file footer_line.hpp
 * @author Radek Vana
 * @brief footer interface
 * @date 2021-04-14
 */

#pragma once
#include "window_frame.hpp"
#include "footer_item_union.hpp" // all possible footer items
#include "footer_def.hpp"

class FooterLine : public AddSuperWindow<window_frame_t> {
    static constexpr size_t max_items = FOOTER_ITEMS_PER_LINE__;
    static constexpr size_t array_sz = max_items + 2; // can add 2 zero rects for centering
    using Rectangles = std::array<Rect16, max_items>;
    static std::array<Rect16::Width_t, array_sz> addBorderZeroWidths(const std::array<Rect16::Width_t, max_items> &source, size_t count);
    size_t storeWidths(std::array<Rect16::Width_t, max_items> &widths) const; // returns count of stored widths
    size_t calculateItemRects(Rect16 *item_rects, Rect16::Width_t *widths, size_t count) const; // returns count of used rectangles
    bool try_split(Rectangles &returned_rects, const std::array<Rect16::Width_t, max_items> &widths, size_t count) const; // single iteration
    size_t split(Rectangles &returned_rects, const std::array<Rect16::Width_t, max_items> &widths, size_t count) const; // split line rectangle into rectangles for items
    void setItemRectangles(Rectangles::iterator rectangles_begin, Rectangles::iterator rectangles_end); // set given rectangles into valid items, show those items and hide the rest
public:
    using IdArray = footer::Record;

private:
    std::array<footer::ItemUnion, max_items> items;
    IdArray item_ids;

public:
    FooterLine(window_t *parent, size_t line_no);

    // create line with items
    template <class... T>
    FooterLine(window_t *parent, size_t line_no, T... args)
        : FooterLine(parent, line_no) {
        Create({ { args... } }, sizeof...(T));
    }

    bool Create(footer::Item item, size_t index);
    void Create(const IdArray &ids, size_t count = FOOTER_ITEMS_PER_LINE__);
    void Erase(size_t index); // index >= max_items erases all
    window_t *SlotAccess(size_t index) const; // footer event might need to access this method, so it must be public
    footer::Item SlotUsedBy(size_t index); // meant to be compared with footer::DecodeItemFromEvent in events
    static constexpr size_t Size() { return max_items; }
    static void SetCenterN(size_t n_and_fewer);
    static size_t GetCenterN();

protected:
    void validatePointers();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    void positionWindows();
    bool slotUsed(size_t index) const;
    void unregister(size_t index);
};
