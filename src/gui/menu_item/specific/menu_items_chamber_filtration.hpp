#pragma once

#include <gui/menu_item/menu_item_select_menu.hpp>

#include <feature/chamber_filtration/chamber_filtration.hpp>

class MI_CHAMBER_FILTRATION_BACKEND : public MenuItemSelectMenu {
public:
    MI_CHAMBER_FILTRATION_BACKEND();

    int item_count() const final {
        return item_count_;
    }

    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;

private:
    buddy::ChamberFiltration::BackendArray items_;
    size_t item_count_ = 0;
};

class MI_CHAMBER_PRINT_FILTRATION_POWER : public WiSpin {
public:
    MI_CHAMBER_PRINT_FILTRATION_POWER();

protected:
    void OnClick() override;
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
