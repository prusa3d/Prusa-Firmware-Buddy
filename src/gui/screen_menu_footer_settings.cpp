/**
 * @file screen_menu_footer_settings.cpp
 * @author Radek Vana
 * @brief settings of menu footer items
 * @date 2021-04-21
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "screen_menus.hpp"
#include "footer_item_union.hpp"
#include "status_footer.hpp"

static constexpr std::array<const char *, 3> labels = { { N_("Item 0"),
    N_("Item 1"),
    N_("Item 2") } };

static constexpr std::array<const char *, size_t(footer::items::count_) + 1> item_labels = { {
    N_("Nozzle"),   //ItemNozzle
    N_("Bed"),      //ItemBed
    N_("Filament"), //ItemFilament
    N_("Speed"),    //ItemSpeed
    N_("LiveZ"),    //ItemLiveZ
    N_("Sheets"),   //ItemSheets
    N_("none")      //count_ == erase
} };

template <size_t INDEX>
class IMiFooter : public WI_SWITCH_t<size_t(footer::items::count_) + 1> {

public:
    IMiFooter()
        : WI_SWITCH_t(size_t(FooterMini::GetSlotInit(INDEX)),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)labels[INDEX]),
            0, is_enabled_t::yes, is_hidden_t::no,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[0]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[1]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[2]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[3]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[4]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[5]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[6])) {}

    virtual void OnChange(size_t old_index) override {
        if (index > size_t(footer::items::count_))
            return; //should not happen
        FooterMini::SetSlotInit(INDEX, footer::items(index));
    }
};

class MI_LEFT_ALIGN_TEMP_ONOFF : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Left align temp.");

public:
    MI_LEFT_ALIGN_TEMP_ONOFF()
        : WI_SWITCH_OFF_ON_t(FooterItemHeater::IsLeftAligned() ? 1 : 0,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? FooterItemHeater::LeftAlign() : FooterItemHeater::ConstPositions();
    }
};

using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN, MI_LEFT_ALIGN_TEMP_ONOFF, IMiFooter<0>, IMiFooter<1>, IMiFooter<2>>;

class ScreenMenuFooterSettings : public Screen {
public:
    constexpr static const char *label = N_("FOOTER");
    ScreenMenuFooterSettings()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuFooterSettings() {
    return ScreenFactory::Screen<ScreenMenuFooterSettings>();
}
