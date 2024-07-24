#pragma once

#include <variant>
#include <cstdint>

#include "screen_toolhead_settings_common.hpp"

namespace screen_toolhead_settings {

class MI_NOZZLE_DIAMETER final : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_NOZZLE_DIAMETER> {
public:
    MI_NOZZLE_DIAMETER(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_NOZZLE_DIAMETER_HELP : public IWindowMenuItem {
public:
    MI_NOZZLE_DIAMETER_HELP();
    void click(IWindowMenu &menu) override;
};

using ScreenToolheadDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_NOZZLE_DIAMETER
#if PRINTER_IS_PRUSA_XL()
    // Prusa XL was sold with .6mm nozzles and then with .4mm nozzles, so the users need to set in the FW what nozzles they have
    // This is to help them out a bit
    ,
    MI_NOZZLE_DIAMETER_HELP
#endif
    >;

class ScreenToolheadDetail : public ScreenToolheadDetail_ {
public:
    ScreenToolheadDetail(Toolhead toolhead = default_toolhead);

private:
    const Toolhead toolhead;
    StringViewUtf8Parameters<2> title_params;
};

class MI_TOOLHEAD : public IWindowMenuItem {
public:
    MI_TOOLHEAD(Toolhead toolhead);

protected:
    void click(IWindowMenu &) final;

private:
    const Toolhead toolhead;
    StringViewUtf8Parameters<2> label_params;
};

#if HAS_TOOLCHANGER()
template <typename>
struct ScreenToolheadSettingsList_;

template <size_t... i>
struct ScreenToolheadSettingsList_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, WithConstructorArgs<MI_TOOLHEAD, ToolheadIndex(i)>..., WithConstructorArgs<MI_TOOLHEAD, AllToolheads {}>>;
};

class ScreenToolheadSettingsList : public ScreenToolheadSettingsList_<std::make_index_sequence<toolhead_count>>::T {
public:
    ScreenToolheadSettingsList();
};
#endif

} // namespace screen_toolhead_settings

using ScreenToolheadDetail = screen_toolhead_settings::ScreenToolheadDetail;

#if HAS_TOOLCHANGER()
using ScreenToolheadSettingsList = screen_toolhead_settings::ScreenToolheadSettingsList;
#endif
