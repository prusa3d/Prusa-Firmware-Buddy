/**
 * @file footer_line.hpp
 * @author Radek Vana
 * @brief footer interface
 * @date 2021-04-14
 */

#pragma once
#include "window_frame.hpp"
#include "footer_item_union.hpp" // all possible footer items

class FooterLine : public AddSuperWindow<window_frame_t> {
    static constexpr size_t max_items = GuiDefaults::FooterItemsPerLine;
    static size_t center_N_andFewer;

public:
    using IdArray = std::array<footer::items, max_items>;

private:
    std::array<footer::ItemUnion, max_items> items;
    IdArray item_ids;

public:
    FooterLine(window_t *parent, size_t line_no);

    bool Create(footer::items item, size_t index);
    void Create(const IdArray &ids);
    void Erase(size_t index);               // index >= max_items erases all
    window_t *SlotAccess(size_t index);     // footer event might need to acces this method, so it must be public
    footer::items SlotUsedBy(size_t index); //meant to be compared with footer::DecodeItemFromEvent in events
    static constexpr size_t Size() { return max_items; }

protected:
    void validatePointers();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    void positionWindows();
    bool slotUsed(size_t index);
    void unregister(size_t index);
};
