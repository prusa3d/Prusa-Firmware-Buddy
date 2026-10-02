#include "screen_indx_nozzle_cleaning_tools.hpp"

#include "indx_nozzle_cleaning.hpp"
#include <screen_menu.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>
#include <menu_item/menu_item_toggle_switch.hpp>
#include <marlin_client.hpp>
#include <tool_index.hpp>

namespace {

class MenuItemToolSelect : public MenuItemToggleSwitch {
public:
    MenuItemToolSelect(PhysicalToolIndex tool, uint32_t &selected_mask)
        : MenuItemToggleSwitch(bool(selected_mask & (uint32_t { 1 } << tool.to_raw())), string_view_utf8::MakeNULLSTR())
        , tool_(tool)
        , selected_mask_(selected_mask) {
        SetLabel(_("Tool %i").formatted(label_params_, tool_.display_index()));
    }

protected:
    void toggled(Tristate) override {
        if (value() == Tristate::yes) {
            selected_mask_ |= tool_bit();
        } else {
            selected_mask_ &= ~tool_bit();
        }
    }

private:
    uint32_t tool_bit() const {
        return uint32_t { 1 } << tool_.to_raw();
    }

    const PhysicalToolIndex tool_;
    uint32_t &selected_mask_;
    StringViewUtf8Parameters<4> label_params_;
};

class MenuItemStart : public IWindowMenuItem {
public:
    MenuItemStart(indx_nozzle_cleaning::Mode mode, const uint32_t &selected_mask)
        : IWindowMenuItem(mode == indx_nozzle_cleaning::Mode::manual ? _("Start Manual Cleaning") : _("Start Automatic Cleaning"))
        , mode_(mode)
        , selected_mask_(selected_mask) {}

    void Loop() override {
        set_enabled(selected_mask_ != 0);
    }

protected:
    void click(IWindowMenu &) override {
        marlin_client::gcode_printf("M1988 P%lu S%u", static_cast<unsigned long>(selected_mask_), static_cast<unsigned>(mode_));
    }

private:
    const indx_nozzle_cleaning::Mode mode_;
    const uint32_t &selected_mask_;
};

enum class Item {
    return_,
    tool,
    start_automatic,
    start_manual,
};

static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
    Item::return_,
    { Item::tool, DynamicIndexMappingType::dynamic_section },
    Item::start_automatic,
    Item::start_manual,
});

class MenuNozzleCleaningTools : public WindowMenuVirtual {
public:
    MenuNozzleCleaningTools(window_frame_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            enabled_tools_[enabled_tool_count_++] = tool.to_raw();
            selected_mask_ |= uint32_t { 1 } << tool.to_raw();
        }
        index_mapping_.set_section_size<Item::tool>(enabled_tool_count_);
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

        case Item::tool:
            variant.emplace<MenuItemToolSelect>(PhysicalToolIndex::from_raw(enabled_tools_[m.pos_in_section]), selected_mask_);
            break;

        case Item::start_automatic:
            variant.emplace<MenuItemStart>(indx_nozzle_cleaning::Mode::automatic, selected_mask_);
            break;

        case Item::start_manual:
            variant.emplace<MenuItemStart>(indx_nozzle_cleaning::Mode::manual, selected_mask_);
            break;
        }
    }

private:
    DynamicIndexMapping<index_mapping_items> index_mapping_;
    std::array<uint8_t, PhysicalToolIndex::count> enabled_tools_ {};
    uint8_t enabled_tool_count_ = 0;
    uint32_t selected_mask_ = 0;
};

class ScreenNozzleCleaningTools : public ScreenMenuBase<MenuNozzleCleaningTools> {
public:
    ScreenNozzleCleaningTools()
        : ScreenMenuBase(nullptr, _("NOZZLE CLEANING"), EFooter::Off) {}
};

} // namespace

ScreenFactory::Creator screen_indx_nozzle_cleaning_tools_creator() {
    return ScreenFactory::Screen<ScreenNozzleCleaningTools>;
}
