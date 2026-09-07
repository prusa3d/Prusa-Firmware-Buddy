#include <screen_fan_selftest.hpp>

#include <i18n.h>
#include <img_resources.hpp>
#include <array>
#include <guiconfig/wizard_config.hpp>
#include <window_wizard_icon.hpp>
#include <window_wizard_progress.hpp>
#include <window_text.hpp>
#include <status_footer.hpp>
#include <common/printer_model.hpp>

#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_switched_fan_test.h>
#include <timing.h>
#include <config_store/store_instance.hpp>
#include <selftest_fans.hpp>

#include <option/xl_enclosure_support.h>
#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
#endif

#include <option/has_chamber_api.h>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

#include <option/has_chamber_filtration_api.h>

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

#include <option/has_bed_fan.h>
#include <option/has_psu_fan.h>
#include <option/has_cpu_fan.h>
#include <option/has_xl_can.h>

using namespace fan_selftest;

namespace {

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;
static constexpr size_t tool_icon_count = option::has_indx ? 1 : PhysicalToolIndex::count;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h;
static constexpr size_t row_5 = row_4 + WizardDefaults::row_h;
static constexpr size_t row_6 = row_5 + WizardDefaults::row_h;
static constexpr size_t row_7 = row_6 + WizardDefaults::row_h;
static constexpr size_t row_8 = row_7 + WizardDefaults::row_h + 20;

static constexpr const char *en_text_header = N_("SELFTEST");
static constexpr const char *en_text_fan_test = N_("Fan RPM tests");
static constexpr const char *en_text_hotend_fan = N_("Hotend fan");
static constexpr const char *en_text_print_fan = N_("Print fan");
#if HAS_INDX()
static constexpr const char *en_text_dock_fan = N_("Dock fan");
#endif
#if HAS_SWITCHED_FAN_TEST()
static constexpr const char *en_text_fans_switched = N_("Switched fans");
static constexpr const char *en_text_info_switched = N_("Based on the test it looks like the fans connectors are switched. Double check your wiring and repeat the test.");
#endif /* HAS_SWITCHED_FAN_TEST() */
#if HAS_CHAMBER_API()
static constexpr const char *en_text_enclosure_fan = N_("Enclosure fan");
static constexpr const char *en_text_cooling_fans = N_("Cooling fans");
static constexpr const char *en_text_filtration_fan = N_("Filtration fan");

#endif

#if HAS_BED_FAN()
static constexpr const char *en_text_bed_fan = N_("Bed fans");
#endif
#if HAS_PSU_FAN()
static constexpr const char *en_text_psu_fan = N_("PSU fan");
#endif
#if HAS_CPU_FAN()
static constexpr const char *en_text_cpu_fan = N_("CPU fan");
#endif
#if HAS_XL_CAN()
static constexpr const char *en_text_bed_mcu_fan = N_("Bed MCU fan");
#endif

#if PRINTER_IS_PRUSA_MK3_5()
static constexpr const char *en_text_manual_check_hotend = N_("Is Hotend fan (left) spinning?");
#endif

static constexpr const char *en_text_test_info = N_("Testing fan rotation on %d%% power, please wait.");
static constexpr const char *en_text_result_ok = N_("All tests passed successfully.");
static constexpr const char *en_text_info_rpm_failed = N_("The RPM test has failed, check both fans are free to spin and connected correctly.");

WindowIconOkNgArray make_fan_icon_array(window_t *parent, int16_t row, size_t icon_cnt) {
    return WindowIconOkNgArray(parent, point_i16_t(int16_t(col_results - (icon_cnt - 1) * WindowIconOkNgArray::icon_space_width), row), icon_cnt, SelftestSubtestState_t::running);
}

namespace frame {
    class SelftestProgress {

        window_wizard_progress_t progress;
        FooterLine footer;

        window_text_t test_title;
        window_text_t print_label;
        window_icon_t print_label_icon;
        window_text_t heatbreak_label;
        window_icon_t heatbreak_label_icon;
        window_text_t info;

        WindowIconOkNgArray print_icons;
        WindowIconOkNgArray heatbreak_icons;

#if HAS_INDX()
        window_text_t dock_fan_label;
        window_icon_t dock_fan_label_icon;
        WindowIconOkNgArray dock_fan_icons;
#endif

        StringViewUtf8Parameters<4> info_params;

#if HAS_CHAMBER_API()
        window_text_t enclosure_label;
        window_icon_t enclosure_label_icon;
        WindowIconOkNgArray enclosure_icons;
#endif

#if HAS_SWITCHED_FAN_TEST()
        WindowIconOkNgArray switched_fan_icons;
        window_text_t switched_fan_label;
#endif

#if HAS_BED_FAN()
        window_text_t bed_fan_label;
        WindowIconOkNgArray bed_fan_icons;
#endif

#if HAS_PSU_FAN()
        window_text_t psu_fan_label;
        WindowIconOkNgArray psu_fan_icons;
#endif

#if HAS_CPU_FAN()
        window_text_t cpu_fan_label;
        window_icon_t cpu_fan_label_icon;
        WindowIconOkNgArray cpu_fan_icons;
#endif

#if HAS_XL_CAN()
        window_text_t bed_mcu_fan_label;
        window_icon_t bed_mcu_fan_label_icon;
        WindowIconOkNgArray bed_mcu_fan_icons;
#endif

        void show_results() {
            bool failed = false;
#if HAS_SWITCHED_FAN_TEST()
            bool switched_fans = false;
#endif
            auto process_fan_result = [&failed](auto result, auto &icons, auto index) {
                const bool subtest_failed = result == TestResult::failed;
                icons.SetState(subtest_failed ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok, index);
                failed |= subtest_failed;
                return subtest_failed;
            };

            const SelftestResult result = config_store().selftest_result.get();
            for (uint8_t i = 0; i < tool_icon_count; i++) {
                const auto tool = PhysicalToolIndex::from_raw(i);
                if (!option::has_indx && !tool.is_enabled()) {
                    continue;
                }
#if HAS_SWITCHED_FAN_TEST()
                if (process_fan_result(result.get_fans_switched(tool), switched_fan_icons, tool.to_raw())) {
                    print_icons.SetState(SelftestSubtestState_t::not_good, tool.to_raw());
                    heatbreak_icons.SetState(SelftestSubtestState_t::not_good, tool.to_raw());
                    switched_fans = true;
                    continue;
                }
#endif
                process_fan_result(result.get_print_fan(tool), print_icons, tool.to_raw());
                process_fan_result(result.get_heatbreak_fan(tool), heatbreak_icons, tool.to_raw());
            }

#if HAS_INDX()
            process_fan_result(result.get_dock_fan(), dock_fan_icons, 0);
#endif

#if HAS_CHAMBER_API()
            switch (buddy::chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
            case buddy::Chamber::Backend::xl_enclosure:
                process_fan_result(config_store().xl_enclosure_fan_selftest_result.get(), enclosure_icons, 0);
                break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
            case buddy::Chamber::Backend::xbuddy_extension:
                static_assert(HAS_CHAMBER_FILTRATION_API());
                if (buddy::xbuddy_extension().using_filtration_fan_instead_of_cooling_fans()) {
                    process_fan_result(config_store().xbe_fan_test_results.get().fans[2], enclosure_icons, 0 /* icon_index */);
                } else {
                    process_fan_result(config_store().xbe_fan_test_results.get().fans[0], enclosure_icons, 0 /* icon_index */);
                    process_fan_result(config_store().xbe_fan_test_results.get().fans[1], enclosure_icons, 1);
                }
                break;
    #endif

            case buddy::Chamber::Backend::none:
                break;
            }
#endif /* HAS_CHAMBER_API() */

#if HAS_BED_FAN()
            const auto bed_fan_results = config_store().bed_fan_selftest_result.get();
            process_fan_result(bed_fan_results.fans[0], bed_fan_icons, 0);
            process_fan_result(bed_fan_results.fans[1], bed_fan_icons, 1);
#endif
#if HAS_PSU_FAN()
            process_fan_result(config_store().psu_fan_selftest_result.get(), psu_fan_icons, 0);
#endif
#if HAS_CPU_FAN()
            if (PrinterModelInfo::current().model == PrinterModel::xls) {
                process_fan_result(config_store().cpu_fan_selftest_result.get(), cpu_fan_icons, 0);
            }
#endif
#if HAS_XL_CAN()
            if (PrinterModelInfo::current().model == PrinterModel::xls) {
                process_fan_result(config_store().bed_mcu_fan_selftest_result.get(), bed_mcu_fan_icons, 0);
            }
#endif

#if HAS_SWITCHED_FAN_TEST()
            if (switched_fans) {
                info.SetText(_(en_text_info_switched));
            } else
#endif
            {
                info.SetText(failed ? _(en_text_info_rpm_failed) : _(en_text_result_ok));
            }
        }

    public:
        explicit SelftestProgress(window_frame_t *parent, PhasesFansSelftest phase)
            // The #ifs screw up the autoformatter
            // clang-format off
            : progress { parent, WizardDefaults::row_1 }
#if HAS_TOOLCHANGER()
            // when toolchanger is enabled, do not show footer with fan RPM, because its likely that no tool will be picked and it would just show zero RPM
            , footer(parent, 0)
#else
            , footer(parent, 0, footer::Item::print_fan, footer::Item::heatbreak_fan)
#endif
            , test_title { parent, Rect16(WizardDefaults::col_0, WizardDefaults::row_0, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fan_test) }
            , print_label { parent, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan) }
            , print_label_icon { parent, &img::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }) }
            , heatbreak_label { parent, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan) }
            , heatbreak_label_icon { parent, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }) }
            , info { parent, Rect16(col_texts, row_8, col_texts_w, WizardDefaults::row_h * 4), is_multiline::yes, is_closed_on_click_t::no }
            , print_icons { make_fan_icon_array(parent, row_2, tool_icon_count) }
            , heatbreak_icons { make_fan_icon_array(parent, row_3, tool_icon_count) }
#if HAS_INDX()
            , dock_fan_label { parent, Rect16(col_texts, row_5, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_dock_fan) }
            , dock_fan_label_icon { parent, &img::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_5 }) }
            , dock_fan_icons { make_fan_icon_array(parent, row_5, 1) }
#endif
#if HAS_CHAMBER_API()
            , enclosure_label { parent, Rect16(col_texts, row_4, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_enclosure_fan) }
            , enclosure_label_icon { parent, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_4 }) }
            , enclosure_icons { make_fan_icon_array(parent, row_4, 1) }
#endif
#if HAS_SWITCHED_FAN_TEST()
            , switched_fan_icons { make_fan_icon_array(parent, row_5, PhysicalToolIndex::count) }
            , switched_fan_label { parent, Rect16(col_texts, row_5, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fans_switched) }
#endif
#if HAS_BED_FAN()
            , bed_fan_label { parent, Rect16( col_texts, row_6, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_bed_fan) }
            , bed_fan_icons { make_fan_icon_array(parent, row_6, 2) }
#endif
#if HAS_PSU_FAN()
            , psu_fan_label { parent, Rect16( col_texts, row_7, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_psu_fan) }
            , psu_fan_icons { make_fan_icon_array(parent, row_7, 1) }
#endif
#if HAS_CPU_FAN()
            , cpu_fan_label { parent, Rect16(col_texts, row_5, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_cpu_fan) }
            , cpu_fan_label_icon { parent, &img::cpu_16x16, point_i16_t({ WizardDefaults::col_0, row_5 }) }
            , cpu_fan_icons { make_fan_icon_array(parent, row_5, 1) }
#endif
#if HAS_XL_CAN()
            , bed_mcu_fan_label { parent, Rect16(col_texts, row_6, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_bed_mcu_fan) }
            , bed_mcu_fan_label_icon { parent, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_6 }) }
            , bed_mcu_fan_icons { make_fan_icon_array(parent, row_6, 1) }
#endif
        // clang-format on
        {
#if HAS_MINI_DISPLAY()
            test_title.set_font(Font::small);
            print_label.set_font(Font::small);
            heatbreak_label.set_font(Font::small);
#endif
#if HAS_TOOLCHANGER() && !HAS_INDX()
            for (auto tool : PhysicalToolIndex::all()) {
                if (!tool.is_enabled()) {
                    print_icons.SetIconHidden(tool.to_raw(), true);
                    heatbreak_icons.SetIconHidden(tool.to_raw(), true);
    #if HAS_SWITCHED_FAN_TEST()
                    // #error dead code found by automatic analyses (see BFW-5461)
                    swtiched_fan_icons.SetIconHidden(tool.to_raw(), true);
    #endif
                }
            }
#endif

#if HAS_CHAMBER_API()
            uint8_t enclosure_fan_count = 0;

            switch (buddy::chamber().backend()) {

            case buddy::Chamber::Backend::none:
                break;

    #if XL_ENCLOSURE_SUPPORT()
            case buddy::Chamber::Backend::xl_enclosure:
                enclosure_fan_count = 1;
                break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
            case buddy::Chamber::Backend::xbuddy_extension:
                static_assert(HAS_CHAMBER_FILTRATION_API());
                if (buddy::xbuddy_extension().using_filtration_fan_instead_of_cooling_fans()) {
                    enclosure_fan_count = 1;
                    enclosure_label.SetText(_(en_text_filtration_fan));
                } else {
                    enclosure_fan_count = 2;
                    enclosure_label.SetText(_(en_text_cooling_fans));
                }
                break;
    #endif
            }

            if (enclosure_fan_count > 0) {
                enclosure_icons.SetIconCount(enclosure_fan_count);
            } else {
                enclosure_label.Hide();
                enclosure_label_icon.Hide();
                enclosure_icons.Hide();
            }

#elif HAS_CHAMBER_FILTRATION_API()
    #error Enclosure filtration fan selftest should be implemented

#endif /* HAS_CHAMBER_API() */

#if HAS_CPU_FAN()
            // CPU fan only physically exists on XLS; hide the row on plain XL.
            if (PrinterModelInfo::current().model != PrinterModel::xls) {
                cpu_fan_label.Hide();
                cpu_fan_label_icon.Hide();
                cpu_fan_icons.Hide();
            }
#endif

#if HAS_XL_CAN()
            // Modular Bed cooling fan lives on the XL-CAN bridge, present only
            // on XLS; hide the row on plain XL.
            if (PrinterModelInfo::current().model != PrinterModel::xls) {
                bed_mcu_fan_label.Hide();
                bed_mcu_fan_label_icon.Hide();
                bed_mcu_fan_icons.Hide();
            }
#endif

            switch (phase) {
            case PhasesFansSelftest::test_100_percent:
                info.SetText(_(en_text_test_info).formatted(info_params, 100));
                break;
            case PhasesFansSelftest::test_40_percent:
                info.SetText(_(en_text_test_info).formatted(info_params, 40));
                break;
            case PhasesFansSelftest::results:
                show_results();
                break;
            default:
                break;
            }
        }

        void update(const fsm::PhaseData &data) {
            progress.set_progress_percent(static_cast<float>(data[0]));
        }
    };

#if PRINTER_IS_PRUSA_MK3_5()
    class ManualCheck {
        window_text_t question;
        window_icon_t fan_icon;
        RadioButtonFSM radio;

    public:
        explicit ManualCheck(window_t *parent, [[maybe_unused]] PhasesFansSelftest phase)
            : question { parent, Rect16(GuiDefaults::MessageTextRect), is_multiline::yes, is_closed_on_click_t::no }
            , fan_icon { parent, &img::fan_error_48x48, GuiDefaults::MessageIconRect.TopLeft() }
            , radio { parent, GuiDefaults::GetButtonRect(GuiDefaults::RectScreenBody), PhasesFansSelftest::manual_check } {
            question.SetText(_(en_text_manual_check_hotend));
            static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio);
        }
    };
#endif

} // namespace frame

using Frames = FrameDefinitionList<ScreenFanSelftest::FrameStorage,
    FrameDefinition<PhasesFansSelftest::test_100_percent, frame::SelftestProgress>,
#if PRINTER_IS_PRUSA_MK3_5()
    FrameDefinition<PhasesFansSelftest::manual_check, frame::ManualCheck>,
#endif
    FrameDefinition<PhasesFansSelftest::test_40_percent, frame::SelftestProgress>,
    FrameDefinition<PhasesFansSelftest::results, frame::SelftestProgress>>;

} // namespace

ScreenFanSelftest::ScreenFanSelftest()
    : ScreenFSM(en_text_header, GuiDefaults::RectScreenBody) {
    header.SetIcon(&img::selftest_16x16);
    create_frame();
}

ScreenFanSelftest::~ScreenFanSelftest() {
    destroy_frame();
}

void ScreenFanSelftest::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame, get_phase());
}

void ScreenFanSelftest::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenFanSelftest::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
