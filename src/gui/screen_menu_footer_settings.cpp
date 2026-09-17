/// @file
#include <screen_menu_footer_settings.hpp>

#include <common/footer_def.hpp>
#include <common/utils/algorithm_extensions.hpp>
#include <status_footer.hpp>
#include <MItem_tools.hpp>
#include <WindowMenuItems.hpp>
#include <gui/screen/screen_menu_virtual.hpp>

static_assert(std::to_underlying(footer::Item::none) == 0, "Current implementation relies on none being 0");

MI_FOOTER::MI_FOOTER(int item)
    : MenuItemSelectMenu({})
    , item_(item) //
{
    SetLabel(_("Item %i").formatted(label_params_, item + 1));
    set_current_item(stdext::index_of(footer::item_list, StatusFooter::GetSlotInit(item_)));
}

int MI_FOOTER::item_count() const {
    return footer::item_list.size();
}

string_view_utf8 MI_FOOTER::build_item_text(int index, [[maybe_unused]] StringViewUtf8ParamBase &params) const {
    return _(footer::to_string(footer::item_list[index]));
}

bool MI_FOOTER::on_item_selected(const OnItemSelectedArgs &args) {
    StatusFooter::SetSlotInit(item_, footer::item_list[args.new_index]);

    return true;
}

namespace {
constexpr screen_menu_virtual::Configuration footer_settings_configuration {
    .item_count = [] -> int { return FOOTER_ITEMS_PER_LINE__ + 2; }, // + MI_RETURN + MI_FOOTER_RESET
    .item_constructor = [](WindowMenuVirtual::ItemVariant &variant, int index) {
        if (index == 0) {
            variant.emplace<MI_RETURN>();
        } else if (index <= FOOTER_ITEMS_PER_LINE__) {
            variant.emplace<MI_FOOTER>(index - 1);
        } else {
            variant.emplace<MI_FOOTER_RESET>();
        } //
    },
    .title = N_("FOOTER"),
    .footer = EFooter::On,
};
} // namespace

MI_FOOTER_SETTINGS::MI_FOOTER_SETTINGS()
    : MI_SCREEN_BASE(ScreenFactory::ScreenWithArg<ScreenMenuVirtual>(&footer_settings_configuration), N_("Footer")) {
}
