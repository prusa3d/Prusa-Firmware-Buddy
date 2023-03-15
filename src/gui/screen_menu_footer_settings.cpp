/**
 * @file screen_menu_footer_settings.cpp
 */

#include "screen_menu_footer_settings.hpp"
#include "footer_item_union.hpp"
#include "status_footer.hpp"
#include "menu_spin_config.hpp"
#include "footer_eeprom.hpp"
#include "DialogMoveZ.hpp"
#include "footer_def.hpp"
#include <option/has_side_fsensor.h>

static constexpr std::array<const char *, FOOTER_ITEMS_PER_LINE__> labels = { { N_("Item 1")
#if FOOTER_ITEMS_PER_LINE__ > 1
                                                                                    ,
    N_("Item 2")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 2
        ,
    N_("Item 3")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 3
        ,
    N_("Item 4")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 4
        ,
    N_("Item 5")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 5
        ,
    N_("Item 6")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 6
        ,
    N_("Item 7")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 7
        ,
    N_("Item 8")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 8
        ,
    N_("Item 9")
#endif
#if FOOTER_ITEMS_PER_LINE__ > 9
        ,
    N_("Item 10")
#endif
} };

IMiFooter::IMiFooter(size_t index_)
    : WI_SWITCH_t(size_t(StatusFooter::GetSlotInit(index_)),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)labels[index_]),
        nullptr, is_enabled_t::yes, is_hidden_t::no,
        // TODO modify ctor to accept an array
        FooterItemNozzle::GetName(),
        FooterItemBed::GetName(),
        FooterItemFilament::GetName(),
        FooterItemFSensor::GetName(),
        FooterItemSpeed::GetName(),
        FooterItemAxisX::GetName(),
        FooterItemAxisY::GetName(),
        FooterItemAxisZ::GetName(),
        FooterItemZHeight::GetName(),
        FooterItemPrintFan::GetName(),
        FooterItemHeatBreakFan::GetName(),
#if defined(FOOTER_HAS_LIVE_Z)
        FooterItemLiveZ::GetName(),
#endif // FOOTER_HAS_LIVE_Z
        FooterItemHeatBreak::GetName(),
#if defined(FOOTER_HAS_SHEETS)
        FooterItemSheets::GetName(),
#endif // FOOTER_HAS_SHEETS
#if HAS_MMU2
        FooterItemFinda::GetName(),
#endif
#if defined(FOOTER_HAS_TOOL_NR)
        FooterItemCurrentTool::GetName(),
        FooterItemAllNozzles::GetName(),
#endif
#if HAS_SIDE_FSENSOR()
        FooterItemFSensorSide::GetName(),
#endif /*HAS_SIDE_FSENSOR()*/
        _("none")) {
}

MI_LEFT_ALIGN_TEMP::MI_LEFT_ALIGN_TEMP()
    : WI_SWITCH_t(size_t(FooterItemHeater::GetDrawType()),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no,
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_0),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_1),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)str_2)) {}

void MI_LEFT_ALIGN_TEMP::OnChange(size_t /*old_index*/) {
    FooterItemHeater::SetDrawType(footer::ItemDrawType(index));
}

MI_SHOW_ZERO_TEMP_TARGET::MI_SHOW_ZERO_TEMP_TARGET()
    : WI_ICON_SWITCH_OFF_ON_t(FooterItemHeater::IsZeroTargetDrawn(),
        string_view_utf8::MakeCPUFLASH((const uint8_t *)label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SHOW_ZERO_TEMP_TARGET::OnChange(size_t old_index) {
    old_index == 0 ? FooterItemHeater::EnableDrawZeroTarget() : FooterItemHeater::DisableDrawZeroTarget();
}

MI_FOOTER_CENTER_N::MI_FOOTER_CENTER_N()
    : WiSpinInt(uint8_t(FooterLine::GetCenterN()),
        SpinCnf::footer_center_N_range, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_FOOTER_CENTER_N::OnClick() {
    FooterLine::SetCenterN(GetVal());
}

void ScreenMenuFooterSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuFooterSettings::ScreenMenuFooterSettings()
    : ScreenMenuFooterSettings__(_(label)) {
    EnableLongHoldScreenAction();
}

ScreenMenuFooterSettingsAdv::ScreenMenuFooterSettingsAdv()
    : ScreenMenuFooterSettingsAdv__(_(label)) {
}
