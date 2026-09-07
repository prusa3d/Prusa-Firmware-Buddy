/// @file
#pragma once

#include <variant>

#include <ScreenFactory.hpp>
#include <i_window_menu_item.hpp>
#include <tool_index.hpp>

namespace buddy::openprinttag {

class MI_OPT_READ_TAG final : public IWindowMenuItem {

public:
    using Tool = std::variant<VirtualToolIndex, AllTools>;

    MI_OPT_READ_TAG(Tool tool = AllTools {});

    void click(IWindowMenu &) override;
    void Loop() override;

private:
    Tool tool_;
    VirtualToolIndex::DisplayNameParams label_params_;
};

}; // namespace buddy::openprinttag
