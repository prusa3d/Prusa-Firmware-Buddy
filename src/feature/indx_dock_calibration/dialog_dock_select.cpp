/// @file
#include "dialog_dock_select.hpp"

#include <bitset>
#include <cstdio>
#include <window_menu_adv.hpp>
#include <window_menu_virtual.hpp>
#include <WindowMenuItems.hpp>
#include <img_resources.hpp>
#include <WindowMenuSwitch.hpp>
#include <ScreenHandler.hpp>
#include <window_header.hpp>
#include <dynamic_index_mapping.hpp>
#include <config_store/store_instance.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <tool_index.hpp>
#include <i18n.h>

namespace {

using DockMask = std::bitset<PhysicalToolIndex::count>;

enum class Item {
    return_,
    dock,
    continue_,
};

static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
    { Item::return_ },
    { Item::dock, DynamicIndexMappingType::dynamic_section },
    { Item::continue_ },
});

class SelectDocksMenu;

/// Per-dock action, indices match dock_toggle_items below
enum class DockAction : uint8_t {
    keep,
    calibrate,
    invalidate,
};

static constexpr const char *dock_toggle_items[] {
    N_("Keep"),
    N_("Calibrate"),
    N_("Invalidate"),
};

class MI_DOCK_TOGGLE final : public MenuItemSwitch {

public:
    MI_DOCK_TOGGLE(SelectDocksMenu &menu, PhysicalToolIndex tool, bool already_calibrated, DockAction action)
        : MenuItemSwitch(string_view_utf8 {}, dock_toggle_items, static_cast<size_t>(action))
        , menu_(menu)
        , dock_(tool) {
        // have to use quick_cycle eventhough we have 3 items to avoid stack overflow
        set_behavior(Behavior::quick_cycle);
        SetLabel(_("Dock %i").formatted(label_params_, tool.display_index()));
        SetIconId(already_calibrated ? &img::ok_color_16x16 : &img::nok_color_16x16);
    }

protected:
    void OnChange(size_t) override;

    virtual Color resolved_value_text_color([[maybe_unused]] Color base_color) const override {
        Color action_color = COLOR_GRAY;
        switch (static_cast<DockAction>(current_item())) {
        case DockAction::keep:
            action_color = COLOR_GRAY;
            break;
        case DockAction::calibrate:
            action_color = COLOR_LIGHT_GREEN;
            break;
        case DockAction::invalidate:
            action_color = COLOR_ORANGE;
            break;
        }

        return action_color;
    }

private:
    SelectDocksMenu &menu_;
    const PhysicalToolIndex dock_;
    StringViewUtf8Parameters<4> label_params_;
};

class MI_CONTINUE final : public IWindowMenuItem {

public:
    MI_CONTINUE(SelectDocksMenu &menu)
        : IWindowMenuItem(_("Continue"))
        , menu_(menu) {}

protected:
    void click(IWindowMenu &) override;

private:
    SelectDocksMenu &menu_;
};

class SelectDocksMenu : public WindowMenuVirtual {
    friend class MI_DOCK_TOGGLE;
    friend class MI_CONTINUE;

public:
    SelectDocksMenu(window_frame_t *parent, Rect16 rect, uint8_t dock_count, bool preselect_all)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes)
        , calibrated_docks(config_store().indx_dock_calibrated_mask.get())
        , dock_count_(dock_count) {

        for (uint8_t i = 0; i < dock_count_; ++i) {
            // Known dock count (4/8): default every dock to Calibrate.
            // "Other": default only uncalibrated docks to Calibrate, keep the rest.
            actions_[i] = (preselect_all || !calibrated_docks.test(i)) ? DockAction::calibrate : DockAction::keep;
        }

        index_mapping_.set_section_size<Item::dock>(dock_count_);
        setup_items();
    }

    int item_count() const final {
        return index_mapping_.total_item_count();
    }

    void setup_item(ItemVariant &variant, int index) final {
        const auto m = index_mapping_.from_index(index);

        switch (m.item) {

        case Item::return_:
            variant.emplace<MI_RETURN>();
            break;

        case Item::dock: {
            const auto dock_index = nth_dock(m.pos_in_section);
            const auto raw = dock_index.to_raw();
            variant.emplace<MI_DOCK_TOGGLE>(*this, dock_index, calibrated_docks.test(raw), actions_[raw]);
            break;
        }

        case Item::continue_:
            variant.emplace<MI_CONTINUE>(*this);
            break;
        }
    }

    void set_action(PhysicalToolIndex dock, DockAction action) {
        actions_[dock.to_raw()] = action;
    }

    /// Result: nullopt = aborted, otherwise per-dock actions for FSMResponseVariant transfer
    std::optional<DockSelection> result;

private:
    /// Get the dock index for the nth dock in the list
    PhysicalToolIndex nth_dock(int n) const {
        return PhysicalToolIndex::from_raw(n);
    }

    std::array<DockAction, PhysicalToolIndex::count> actions_ {};
    const DockMask calibrated_docks;
    const uint8_t dock_count_;
    DynamicIndexMapping<index_mapping_items> index_mapping_;
};

void MI_DOCK_TOGGLE::OnChange(size_t) {
    menu_.set_action(dock_, static_cast<DockAction>(current_item()));
}

void MI_CONTINUE::click(IWindowMenu &) {
    DockSelection sel;
    for (uint8_t i = 0; i < menu_.dock_count_; ++i) {
        switch (menu_.actions_[i]) {
        case DockAction::keep:
            break;
        case DockAction::calibrate:
            sel.calibrate |= (1 << i);
            break;
        case DockAction::invalidate:
            sel.invalidate |= (1 << i);
            break;
        }
    }
    menu_.result = sel;
    Screens::Access()->Close();
}

class SelectDocksDialog final : public IDialog {

public:
    SelectDocksDialog(uint8_t dock_count, bool preselect_all)
        : header_(this)
        , menu_(this, GuiDefaults::RectScreenNoHeader, dock_count, preselect_all) {
        header_.SetText(_("SELECT DOCKS"));
        CaptureNormalWindow(menu_);
    }

    auto result() const {
        return menu_.menu.result;
    }

private:
    window_header_t header_;
    WindowExtendedMenu<SelectDocksMenu> menu_;
};

} // namespace

std::optional<DockSelection> select_docks_dialog(uint8_t dock_count, bool preselect_all) {
    SelectDocksDialog d(dock_count, preselect_all);
    Screens::Access()->gui_loop_until_dialog_closed();
    return d.result();
}
