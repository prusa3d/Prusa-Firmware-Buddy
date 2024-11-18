#include "MItem_steel_sheets.hpp"

#include <common/SteelSheets.hpp>
#include <common/utils/algorithm_extensions.hpp>

/*****************************************************************************/
// MI_CURRENT_PROFILE
MI_CURRENT_SHEET_PROFILE::MI_CURRENT_SHEET_PROFILE()
    : MenuItemSelectMenu(_("Sheet Profile")) //
{
    for (size_t i = 0; i < items_.size(); i++) {
        if (i != SteelSheets::GetActiveSheetIndex() && !SteelSheets::IsSheetCalibrated(i)) {
            continue;
        }

        items_[item_count_] = i;
        item_count_++;
    }

    set_current_item(stdext::index_of(items_, SteelSheets::GetActiveSheetIndex()));
}

int MI_CURRENT_SHEET_PROFILE::item_count() const {
    return item_count_;
}

void MI_CURRENT_SHEET_PROFILE::build_item_text(int index, const std::span<char> &buffer) const {
    SteelSheets::SheetName(items_[index], buffer);
}

bool MI_CURRENT_SHEET_PROFILE::on_item_selected([[maybe_unused]] int old_index, int new_index) {
    SteelSheets::SelectSheet(items_[new_index]);
    return true;
}
