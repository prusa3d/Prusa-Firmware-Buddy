#include "screen_factory_reset.hpp"

#include <array>

#include <WindowMenuItems.hpp>
#include <version/version.hpp>
#include <window_menu_callback_item.hpp>
#include <common/utils/dynamic_index_mapping.hpp>
#include <img_resources.hpp>
#include <ScreenHandler.hpp>
#include <common/printer_model.hpp>
#include <feature/factory_reset/factory_reset.hpp>

namespace {

using Item = FactoryReset::Item;
using ItemBitset = FactoryReset::ItemBitset;

struct PresetConfig {
    const char *title;

    ItemBitset items_to_keep;
};

constexpr std::array presets {
    PresetConfig { N_("Ownership Transfer"), FactoryReset::ownership_transfer },
    PresetConfig { N_("Fix Common Misconfigurations"), FactoryReset::fix_common_misconfigurations },
    PresetConfig { N_("Keep HW Config"), FactoryReset::keep_hw_config },
    PresetConfig { N_("Shipping Reset"), FactoryReset::shipping_reset },
    PresetConfig { N_("Full Reset"), FactoryReset::full_reset },
};

enum class MenuItem {
    return_,
    hard_reset,
    preset,
};

static constexpr auto menu_items = std::to_array<DynamicIndexMappingRecord<MenuItem>>({
    { MenuItem::return_ },
    { MenuItem::preset, DynamicIndexMappingType::static_section, presets.size() },

    // !!! Hard reset has to be the last item here
    // This way the user can do a factory reset by always scrolling to the bottom and then pressing the knob
    { MenuItem::hard_reset },
});

constexpr DynamicIndexMapping<menu_items> menu_index_mapping;

class MenuItemFactoryResetDetailItem : public MenuItemSwitch {
    static constexpr auto item_texts = std::to_array<const char *>({
        N_("Reset"),
        N_("Keep"),
    });

    const Item item_;
    ItemBitset *const bitset_;

public:
    MenuItemFactoryResetDetailItem(Item item, ItemBitset *bitset)
        : MenuItemSwitch(_(FactoryReset::items_config[item].title), item_texts, bitset->test(std::to_underlying(item)))
        , item_(item)
        , bitset_(bitset) {
        set_behavior(Behavior::quick_cycle);
    }

private:
    virtual void OnChange(size_t) final {
        bitset_->set(std::to_underlying(item_), get_index());
    }

    Color resolved_value_text_color(Color) const override {
        return current_item() ? COLOR_GREEN : COLOR_RED;
    }
};

enum class DetailItem {
    return_,

    /// Section - items where we're deciding what to keep and what to discard
    item,

    /// Do the factory reset
    execute,
};

static constexpr auto detail_items = std::to_array<DynamicIndexMappingRecord<DetailItem>>({
    { DetailItem::return_ },
    { DetailItem::item, DynamicIndexMappingType::static_section, FactoryReset::items_config.size() },
    { DetailItem::execute },
});

constexpr DynamicIndexMapping<detail_items> detail_index_mapping;

/// Menu with selection of which items to keep and which to reset
class WindowMenuFactoryResetDetail final : public WindowMenuVirtual {
public:
    WindowMenuFactoryResetDetail(window_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
    }

    using WindowMenuVirtual::setup_items;

public:
    int item_count() const final {
        return detail_index_mapping.total_item_count();
    }

    ItemBitset items_to_keep;

protected:
    void setup_item(ItemVariant &variant, int index) final {
        const auto mapping = detail_index_mapping.from_index(index);

        switch (mapping.item) {

        case DetailItem::return_: {
            variant.emplace<MI_RETURN>();
            break;
        }

        case DetailItem::item: {
            variant.emplace<MenuItemFactoryResetDetailItem>(static_cast<Item>(mapping.pos_in_section), &items_to_keep);
            break;
        }

        case DetailItem::execute: {
            const auto callback = [this] {
                if (MsgBoxWarning(_("This operation cannot be undone. Your data will be lost!\nDo you want to proceed with the factory reset?"), Responses_YesNo, 1) == Response::Yes) {
                    FactoryReset::perform(false, items_to_keep);
                }
            };
            variant.emplace<WindowMenuCallbackItem>(_("Execute Factory Reset"), callback, &img::ok_16x16);
            break;
        }
        }
    }
};

/// Screen with selection of which items to keep and which to reset, return and execute button
class ScreenFactoryResetDetail final : public ScreenMenuBase<WindowMenuFactoryResetDetail> {
public:
    ScreenFactoryResetDetail(ItemBitset items_to_keep)
        : ScreenMenuBase(nullptr, _("FACTORY RESET"), EFooter::Off) {

        menu.menu.items_to_keep = items_to_keep;
        menu.menu.setup_items();
    }
};

} // namespace

namespace screen_factory_reset {

WindowMenuFactoryReset::WindowMenuFactoryReset(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
    setup_items();
}

int WindowMenuFactoryReset::item_count() const {
    return menu_index_mapping.total_item_count();
}

void WindowMenuFactoryReset::setup_item(ItemVariant &variant, int index) {
    const auto mapping = menu_index_mapping.from_index(index);
    switch (mapping.item) {

    case MenuItem::return_: {
        variant.emplace<MI_RETURN>();
        break;
    }

    case MenuItem::hard_reset: {
        const auto callback = [] {
            StringViewUtf8Parameters<24> params;
            static constexpr const char *msg = N_(
                "This operation cannot be undone. Current configuration will be lost!"
                "\nYou will need a USB drive with this firmware (%s_firmware_%s.bbf file) to start the printer again."
                "\nDo you really want to continue?");
            const string_view_utf8 str = _(msg).formatted(params, PrinterModelInfo::firmware_base().id_str, version::project_version);

            if (MsgBoxWarning(str, Responses_YesNo, 1) == Response::Yes) {
                FactoryReset::perform(true, {});
            }
        };
        variant.emplace<WindowMenuCallbackItem>(_("Hard Reset (USB with FW needed)"), callback);
        break;
    }

    case MenuItem::preset: {
        const auto &preset = presets[mapping.pos_in_section];
        const auto callback = [&preset] {
            Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenFactoryResetDetail>(preset.items_to_keep));
        };
        variant.emplace<WindowMenuCallbackItem>(_(preset.title), callback, nullptr, expands_t::yes);
        break;
    }
    }
}

} // namespace screen_factory_reset

ScreenFactoryReset::ScreenFactoryReset()
    : ScreenMenuBase(nullptr, _("FACTORY RESET"), EFooter::Off) {}
