/// @file
#include <screen_menu_footer_settings.hpp>

#include <common/footer_def.hpp>
#include <common/utils/algorithm_extensions.hpp>
#include <status_footer.hpp>

static_assert(std::to_underlying(footer::Item::none) == 0, "Current implementation relies on none being 0");

I_MI_FOOTER::I_MI_FOOTER(int item)
    : MenuItemSelectMenu({})
    , item_(item) //
{
    SetLabel(_("Item %i").formatted(label_params_, item + 1));
    set_current_item(stdext::index_of(footer::item_list, StatusFooter::GetSlotInit(item_)));
}

int I_MI_FOOTER::item_count() const {
    return footer::item_list.size();
}

string_view_utf8 I_MI_FOOTER::build_item_text(int index, [[maybe_unused]] MenuItemSelectMenu::ItemTextParams &params) const {
    return _(footer::to_string(footer::item_list[index]));
}

bool I_MI_FOOTER::on_item_selected(const OnItemSelectedArgs &args) {
    StatusFooter::SetSlotInit(item_, footer::item_list[args.new_index]);

    return true;
}

ScreenMenuFooterSettings::ScreenMenuFooterSettings()
    : ScreenMenuFooterSettings__(_(label)) {
    EnableLongHoldScreenAction();
}
