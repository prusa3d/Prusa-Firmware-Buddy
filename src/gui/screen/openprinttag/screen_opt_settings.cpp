#include "screen_opt_settings.hpp"
#include "screen_opt_tag_list.hpp"

#include <screen_menu.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>

namespace buddy::openprinttag {

using ScreenOPTSettings_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_OPT_READER_STATUS,
    MI_OPT_READ_TAG>;

class ScreenOPTSettings final : public ScreenOPTSettings_ {

public:
    ScreenOPTSettings()
        : ScreenOPTSettings_(_("OPENPRINTTAG SETTINGS")) {}
};

MI_OPT_READER_STATUS::MI_OPT_READER_STATUS()
    // TODO: Tell apart several ANFC nodes (BFW-9360)
    : MenuItemCyphalNodeStatus(_("Reader Status"), xbuddy_extension::NodeName::cz_prusa3d_honeybee_nfc) {}

MI_OPT_SETTINGS::MI_OPT_SETTINGS()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH("OpenPrintTag"), &img::openprinttag_white_16x16) {
    set_show_expand_icon();
}

void MI_OPT_SETTINGS::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenOPTSettings>();
}

}; // namespace buddy::openprinttag
