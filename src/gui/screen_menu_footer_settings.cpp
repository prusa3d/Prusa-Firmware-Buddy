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
#include "menu_spin_config.hpp"

static constexpr std::array<const char *, FOOTER_ITEMS_PER_LINE__> labels = { { N_("Item 0")
#if FOOTER_ITEMS_PER_LINE__ > 1
                                                                                    ,
    N_("Item 1")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
        ,
    N_("Item 2")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
        ,
    N_("Item 3")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
        ,
    N_("Item 4")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 5
        ,
    N_("Item 5")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 6
        ,
    N_("Item 6")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 7
        ,
    N_("Item 7")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 8
        ,
    N_("Item 8")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 9
        ,
    N_("Item 9")
#endif
} };

static constexpr std::array<const char *, size_t(footer::items::count_) + 1> item_labels = { {
    N_("Nozzle"),   //ItemNozzle
    N_("Bed"),      //ItemBed
    N_("Filament"), //ItemFilament
    N_("Speed"),    //ItemSpeed
#if defined(FOOTER_HAS_LIVE_Z)
    N_("LiveZ"), //ItemLiveZ
#endif           // FOOTER_HAS_LIVE_Z
#if defined(FOOTER_HAS_SHEETS)
    N_("Sheets"), //ItemSheets
#endif            // FOOTER_HAS_SHEETS
    N_("none")    //count_ == erase
} };

template <size_t INDEX>
class IMiFooter : public WI_SWITCH_t<size_t(footer::items::count_) + 1> {

public:
    IMiFooter()
        : WI_SWITCH_t(size_t(StatusFooter::GetSlotInit(INDEX)),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)labels[INDEX]),
            0, is_enabled_t::yes, is_hidden_t::no,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[0]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[1]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[2]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[3]),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[4])
#if defined(FOOTER_HAS_LIVE_Z) || defined(FOOTER_HAS_SHEETS)
                ,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[5])
#endif // FOOTER_HAS_LIVE_Z || FOOTER_HAS_SHEETS
#if defined(FOOTER_HAS_LIVE_Z) && defined(FOOTER_HAS_SHEETS)
                ,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)item_labels[6])
#endif // FOOTER_HAS_LIVE_Z && FOOTER_HAS_SHEETS
        ) {
    }

    virtual void OnChange(size_t old_index) override {
        if (index > size_t(footer::items::count_))
            return; //should not happen
        StatusFooter::SetSlotInit(INDEX, footer::items(index));
    }
};

class MI_LEFT_ALIGN_TEMP : public WI_SWITCH_t<3> {
    constexpr static const char *const label = N_("Temp. style");
    constexpr static const char *str_0 = "Static";
    constexpr static const char *str_1 = "Static-left";
    constexpr static const char *str_2 = "Dynamic";

public:
    MI_LEFT_ALIGN_TEMP()
        : WI_SWITCH_t(size_t(FooterItemHeater::GetDrawType()),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_0),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_1),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_2)) {}

    virtual void OnChange(size_t /*old_index*/) override {
        FooterItemHeater::SetDrawType(footer::ItemDrawType(index));
    }
};

class MI_SHOW_ZERO_TEMP_TARGET : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Temp. show zero");

public:
    MI_SHOW_ZERO_TEMP_TARGET()
        : WI_SWITCH_OFF_ON_t(FooterItemHeater::IsZeroTargetDrawn(),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no) {}

    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? FooterItemHeater::EnableDrawZeroTarget() : FooterItemHeater::DisableDrawZeroTarget();
    }
};

class MI_FOOTER_CENTER_N : public WI_SPIN_U08_t {
    constexpr static const char *const label = N_("Center N and fewer items");

public:
    MI_FOOTER_CENTER_N()
        : WI_SPIN_U08_t(uint8_t(FooterLine::GetCenterN()),
            SpinCnf::footer_center_N_range, _(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void OnClick() override {
        FooterLine::SetCenterN(GetVal());
    }
};

using Screen = ScreenMenu<EFooter::On, MI_RETURN, MI_FOOTER_CENTER_N, MI_LEFT_ALIGN_TEMP, MI_SHOW_ZERO_TEMP_TARGET, IMiFooter<0>
#if FOOTER_ITEMS_PER_LINE__ > 1
    ,
    IMiFooter<1>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
    ,
    IMiFooter<2>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
    ,
    IMiFooter<3>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
    ,
    IMiFooter<4>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 5
    ,
    IMiFooter<5>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 6
    ,
    IMiFooter<6>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 7
    ,
    IMiFooter<7>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 8
    ,
    IMiFooter<8>
#endif
#if FOOTER_ITEMS_PER_LINE__ > 9
    ,
    IMiFooter<9>
#endif
    >;

class ScreenMenuFooterSettings : public Screen {
public:
    constexpr static const char *label = N_("FOOTER");
    ScreenMenuFooterSettings()
        : Screen(_(label)) {}
};

ScreenFactory::UniquePtr GetScreenMenuFooterSettings() {
    return ScreenFactory::Screen<ScreenMenuFooterSettings>();
}
