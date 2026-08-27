#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>
#include <WindowMenuItems.hpp>

#include <option/xl_enclosure_support.h>
#include <feature/chamber_filtration/chamber_filtration.hpp>

class MI_CHAMBER_FILTRATION_BACKEND : public MenuItemSelectMenu {
public:
    MI_CHAMBER_FILTRATION_BACKEND();

    int item_count() const final {
        return item_count_;
    }

    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;

private:
    buddy::ChamberFiltration::BackendArray items_;
    size_t item_count_ = 0;
};

class MI_CHAMBER_PRINT_FILTRATION : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_CHAMBER_PRINT_FILTRATION();

protected:
    void OnChange(size_t) override;
};

class MI_CHAMBER_PRINT_FILTRATION_POWER : public WiSpin {
public:
    MI_CHAMBER_PRINT_FILTRATION_POWER();

protected:
    void OnClick() override;
    void Loop() override;
};

class MI_CHAMBER_POST_PRINT_FILTRATION : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_CHAMBER_POST_PRINT_FILTRATION();

protected:
    void OnChange(size_t) override;
};

class MI_CHAMBER_POST_PRINT_FILTRATION_POWER : public WiSpin {
public:
    MI_CHAMBER_POST_PRINT_FILTRATION_POWER();

protected:
    void OnClick() override;
    void Loop() override;
};

class MI_CHAMBER_POST_PRINT_FILTRATION_DURATION : public WiSpin {
public:
    MI_CHAMBER_POST_PRINT_FILTRATION_DURATION();

protected:
    void OnClick() override;
    void Loop() override;
};

class MI_CHAMBER_ALWAYS_FILTER : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_CHAMBER_ALWAYS_FILTER();

protected:
    void OnChange(size_t) override;
};

class MI_CHAMBER_FILTER_TIME_USED : public WiSpin {
public:
    MI_CHAMBER_FILTER_TIME_USED();

protected:
    void OnClick() override;
    void Loop() override;
};

class MI_CHAMBER_CHANGE_FILTER : public IWindowMenuItem {
public:
    MI_CHAMBER_CHANGE_FILTER();

protected:
    virtual void click(IWindowMenu &) override;
};
