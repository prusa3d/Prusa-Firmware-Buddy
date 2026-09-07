#include "screen_opt_settings.hpp"
#include "screen_opt_tag_list.hpp"

#include <screen_menu.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>

namespace buddy::openprinttag {

class MI_OPT_AUTO_SCAN_ON_LOAD final : public MenuItemSwitch {
public:
    static constexpr const EnumArray<Tristate::Value, const char *, 3> items {
        { Tristate::no, N_("Do Nothing") },
        { Tristate::yes, N_("Autoscan") },
        { Tristate::other, N_("Ask") },
    };

    MI_OPT_AUTO_SCAN_ON_LOAD()
        : MenuItemSwitch(_("On Filament Load"), items, std::to_underlying(config_store().opt_auto_read_on_load.get().value)) {}

protected:
    virtual void OnChange(size_t) override {
        config_store().opt_auto_read_on_load.set(static_cast<Tristate::Value>(get_index()));
    }
};

using ScreenOPTSettings_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_OPT_AUTO_SCAN_ON_LOAD,
    MI_OPT_READ_TAG>;

class ScreenOPTSettings final : public ScreenOPTSettings_ {

public:
    ScreenOPTSettings()
        : ScreenOPTSettings_(_("OPENPRINTTAG SETTINGS")) {}
};

MI_OPT_SETTINGS::MI_OPT_SETTINGS()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH("OpenPrintTag"), &img::openprinttag_white_16x16) {
    set_show_expand_icon();
}

void MI_OPT_SETTINGS::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenOPTSettings>();
}

}; // namespace buddy::openprinttag
