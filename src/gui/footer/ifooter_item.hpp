/**
 * @file ifooter_item.hpp
 * @author Radek Vana
 * @brief footer item interface
 * @date 2021-03-30
 */

#pragma once
#include "window_frame.hpp"
#include "footer_icon.hpp"
#include "footer_text.hpp"
#include "changed.hpp"
#include "resized.hpp"

class IFooterItem : public AddSuperWindow<window_frame_t> {
    // uint16_t limits period to 65.5s but save 4B RAM
    uint16_t update_period;
    uint16_t last_updated;

public:
    static constexpr char no_tool_str[] = "---"; ///< String shown if no tool is picked
    static constexpr int no_tool_value = std::numeric_limits<int>::min(); ///< Value passed if no tool is picked

    static constexpr size_t item_h = GuiDefaults::FooterItemHeight;
    static constexpr size_t item_top = GuiDefaults::RectFooter.Top();
    static Rect16::Width_t TextWidth(string_view_utf8 text);

    IFooterItem(window_t *parent, Rect16::Width_t width);
    constexpr void ChangeUpdatePeriod(uint16_t ms) { update_period = ms; }

protected:
    enum class TickResult {
        unchanged,
        changed,
        changed_and_resized
    };

    TickResult tick(); // called by event

    // this methods could be handled by text field
    // but we might want to do something with icon too
    // so here it is more general
    virtual resized_t updateState() = 0;
    virtual changed_t updateValue() = 0;

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class IFooterIconText : public AddSuperWindow<IFooterItem> {
public:
    static constexpr size_t item_h = GuiDefaults::FooterItemHeight;

protected:
    FooterIcon icon;
    FooterText text;

public:
    static Rect16::Width_t MeasureTextWidth(string_view_utf8 text);
    IFooterIconText(window_t *parent, const img::Resource *icon, Rect16::W_t width); // icon width is calculated from resource
};

// this class must be able to create stringview
// so it can measure text and correctly create sub windows
// so it contains 2 function pointers
class FooterIconText_IntVal : public AddSuperWindow<IFooterIconText> {
public:
    using view_maker_cb = string_view_utf8 (*)(int val);
    using reader_cb = int (*)();

protected:
    // make view in child can contain static char array (buffer) !!! it is a good thing !!!
    // because all those items can be selected and
    // !!! this only makes best case memory consumption equal to worst case !!!
    // which is great - we should not have memory isues based on combination of selected items
    view_maker_cb makeView;
    reader_cb readCurrentValue;

    int value;

    virtual changed_t updateValue() override;
    virtual resized_t updateState() override;
    static Rect16::Width_t GetTotalWidth(Rect16::Width_t icon_w, string_view_utf8 view);

public:
    FooterIconText_IntVal(window_t *parent, const img::Resource *icon, view_maker_cb view_maker, reader_cb value_reader);
};
class FooterIconText_FloatVal : public AddSuperWindow<IFooterIconText> {
public:
    using view_maker_cb = string_view_utf8 (*)(float val);
    using reader_cb = float (*)();

protected:
    // make view in child can contain static char array (buffer) !!! it is a good thing !!!
    // because all those items can be selected and
    // !!! this only makes best case memory consumption equal to worst case !!!
    // which is great - we should not have memory isues based on combination of selected items
    view_maker_cb makeView;
    reader_cb readCurrentValue;

    float value;

    virtual changed_t updateValue() override;
    virtual resized_t updateState() override;
    static Rect16::Width_t GetTotalWidth(Rect16::Width_t icon_w, string_view_utf8 view);

public:
    FooterIconText_FloatVal(window_t *parent, const img::Resource *icon, view_maker_cb view_maker, reader_cb value_reader);
};
