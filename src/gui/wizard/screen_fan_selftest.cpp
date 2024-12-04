#include <screen_fan_selftest.hpp>

#include <i18n.h>
#include <img_resources.hpp>
#include <array>
#include <guiconfig/wizard_config.hpp>
#include <window_wizard_icon.hpp>
#include <window_wizard_progress.hpp>
#include <window_text.hpp>
#include <status_footer.hpp>
#include <option/has_toolchanger.h>
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

using namespace fan_selftest;

namespace {

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h;
static constexpr size_t row_5 = row_4 + WizardDefaults::row_h;
static constexpr size_t row_6 = row_5 + WizardDefaults::row_h + 20;

static constexpr const char *en_text_header = N_("SELFTEST");
static constexpr const char *en_text_fan_test = N_("Fan Test");
#if HAS_MINI_DISPLAY()
static constexpr const char *en_text_hotend_fan = N_("Hotend fan");
static constexpr const char *en_text_print_fan = N_("Print fan");
static constexpr const char *en_text_fans_switched = N_("Switched fans");
    #if HAS_CHAMBER_API()
static constexpr const char *en_text_enclosure_fan = N_("Enclosure fan");
    #endif
#else
    #if PRINTER_IS_PRUSA_iX()
// for iX with turbine, heatbreak fan eval always succeeds
static constexpr const char *en_text_hotend_fan = N_("Hotend fan RPM test (disabled!)");
    #else
static constexpr const char *en_text_hotend_fan = N_("Hotend fan RPM test");
    #endif
static constexpr const char *en_text_print_fan = N_("Print fan RPM test");
static constexpr const char *en_text_fans_switched = N_("Checking for switched fans");
    #if HAS_CHAMBER_API()
static constexpr const char *en_text_enclosure_fan = N_("Enclosure fan RPM test");
    #endif /* HAS_CHAMBER_API() */
#endif /* HAS_XXXX_DISPLAY() */

#if PRINTER_IS_PRUSA_MK3_5()
static constexpr const char *en_text_manual_check_hotend = N_("Is Hotend fan (left) spinning?");
#endif

static constexpr const char *en_text_test_100_info = N_("Testing fans on 100% RPM, please wait.");
static constexpr const char *en_text_test_40_info = N_("Testing fans on 40% RPM, please wait.");
static constexpr const char *en_text_result_ok = N_("All tests passed successfully.");
static constexpr const char *en_text_info_rpm_failed = N_("The RPM test has failed, check both fans are free to spin and connected correctly.");
#if HAS_SWITCHED_FAN_TEST()
static constexpr const char *en_text_info_switched = N_("Based on the test it looks like the fans connectors are switched. Double check your wiring and repeat the test.");
#endif /* HAS_SWITCHED_FAN_TEST() */

WindowIconOkNgArray make_fan_icon_array(window_t *parent, int16_t row, size_t icon_cnt) {
    return WindowIconOkNgArray(parent, point_i16_t(int16_t(col_results - (icon_cnt - 1) * WindowIconOkNgArray::icon_space_width), row), icon_cnt, SelftestSubtestState_t::running);
}

namespace frame {
    class SelftestProgress : public window_frame_t {

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

#if HAS_CHAMBER_API()
        window_text_t enclosure_label;
        window_icon_t enclosure_label_icon;
        WindowIconOkNgArray enclosure_icons;
#endif

#if HAS_SWITCHED_FAN_TEST()
        WindowIconOkNgArray switched_fan_icons;
        window_text_t switched_fan_label;
#endif

        void show_results() {
            bool failed = false;
#if HAS_SWITCHED_FAN_TEST()
            bool switched_fans = false;
#endif
            auto process_fan_result = [&failed](auto result, auto &icons, auto index) {
                const bool subtest_failed = result == TestResult_Failed;
                icons.SetState(subtest_failed ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok, index);
                failed |= subtest_failed;
                return subtest_failed;
            };

            const SelftestResult result = config_store().selftest_result.get();
            for (size_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
                if (!prusa_toolchanger.is_tool_enabled(i)) {
                    continue;
                }
#endif
#if HAS_SWITCHED_FAN_TEST()
                if (process_fan_result(result.tools[i].fansSwitched, switched_fan_icons, i)) {
                    print_icons.SetState(SelftestSubtestState_t::not_good, i);
                    heatbreak_icons.SetState(SelftestSubtestState_t::not_good, i);
                    switched_fans = true;
                    continue;
                }
#endif
                process_fan_result(result.tools[i].printFan, print_icons, i);
                process_fan_result(result.tools[i].heatBreakFan, heatbreak_icons, i);
            }

#if HAS_CHAMBER_API()
            switch (buddy::chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
            case buddy::Chamber::Backend::xl_enclosure:
                process_fan_result(config_store().xl_enclosure_fan_selftest_result.get(), enclosure_icons, 0);
                break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
            case buddy::Chamber::Backend::xbuddy_extension:
                process_fan_result(config_store().xbe_fan_test_results.get().fans[0], enclosure_icons, 0);
                process_fan_result(config_store().xbe_fan_test_results.get().fans[1], enclosure_icons, 1);
                // Third chamber fan is not yet implemented
                break;
    #endif

            case buddy::Chamber::Backend::none:
                break;
            }
#endif /* HAS_CHAMBER_API() */

#if HAS_SWITCHED_FAN_TEST()
            if (switched_fans) {
                info.SetText(_(en_text_fans_switched));
                return;
            } else
#endif
            {
                info.SetText(failed ? _(en_text_info_rpm_failed) : _(en_text_result_ok));
            }
        }

    public:
        explicit SelftestProgress(window_t *parent, PhasesFansSelftest phase)
            // clang-format off
            : window_frame_t(parent, parent->GetRect())
            , progress { this, WizardDefaults::row_1 }
#if HAS_TOOLCHANGER()
            // when toolchanger is enabled, do not show footer with fan RPM, because its likely that no tool will be picked and it would just show zero RPM
            , footer(this, 0)
#else
            , footer(this, 0, footer::Item::print_fan, footer::Item::heatbreak_fan)
#endif
            , test_title { this, Rect16(col_texts, WizardDefaults::row_0, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fan_test) }
            , print_label { this, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan) }
            , print_label_icon { this, &img::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }) }
            , heatbreak_label { this, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan) }
            , heatbreak_label_icon { this, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }) }
            , info { this, Rect16(col_texts, row_6, col_texts_w, WizardDefaults::txt_h * 2), is_multiline::yes, is_closed_on_click_t::no }
            , print_icons { make_fan_icon_array(this, row_2, HOTENDS) }
            , heatbreak_icons { make_fan_icon_array(this, row_3, HOTENDS) }
#if HAS_CHAMBER_API()
            , enclosure_label { this, Rect16(col_texts, row_4, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_enclosure_fan) }
            , enclosure_label_icon { this, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_4 }) }
            , enclosure_icons { make_fan_icon_array(this, row_4, 1) }
#endif
#if HAS_SWITCHED_FAN_TEST()
            , switched_fan_icons { make_fan_icon_array(this, row_5, HOTENDS) }
            , switched_fan_label { parent, Rect16(col_texts, row_5, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fans_switched) }
#endif
        // clang-format on
        {
#if HAS_TOOLCHANGER()
            for (size_t i = 0; i < HOTENDS; i++) {
                if (!prusa_toolchanger.is_tool_enabled(i)) {
                    print_icons.SetIconHidden(i, true);
                    heatbreak_icons.SetIconHidden(i, true);
    #if HAS_SWITCHED_FAN_TEST()
                    swtiched_fan_icons.SetIconHidden(i, true);
    #endif
                }
            }
#endif

#if HAS_CHAMBER_API()
            switch (buddy::chamber().backend()) {

            case buddy::Chamber::Backend::none:
                enclosure_label.Hide();
                enclosure_label_icon.Hide();
                enclosure_icons.Hide();
                break;

    #if XL_ENCLOSURE_SUPPORT()
            case buddy::Chamber::Backend::xl_enclosure:
                // Set correctly by default in the initializer list (1 fan)
                break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
            case buddy::Chamber::Backend::xbuddy_extension: {
                Rect16 new_rect = enclosure_icons.GetRect();
                // Third chamber fan is not implemented yet
                new_rect = Rect16::Left_t(col_results - WindowIconOkNgArray::icon_space_width);
                new_rect = Rect16::Width_t(WindowIconOkNgArray::icon_space_width * 2);
                enclosure_icons.SetIconCount(2, new_rect);
                break;
            }
    #endif
            }
#endif /* HAS_CHAMBER_API() */

            switch (phase) {
            case PhasesFansSelftest::test_100_percent:
                info.SetText(_(en_text_test_100_info));
                break;
            case PhasesFansSelftest::test_40_percent:
                info.SetText(_(en_text_test_40_info));
                break;
            case PhasesFansSelftest::results:
                show_results();
                break;
            default:
                break;
            }
        }

        void update(const fsm::PhaseData &data) {
            progress.SetProgressPercent(static_cast<float>(data[0]));
        }
    };

#if PRINTER_IS_PRUSA_MK3_5()
    class ManualCheck {
        window_text_t question;
        RadioButtonFSM radio;

    public:
        explicit ManualCheck(window_t *parent, [[maybe_unused]] PhasesFansSelftest phase)
            : question { parent, Rect16(GuiDefaults::MessageTextRect), is_multiline::yes, is_closed_on_click_t::no }
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
    : ScreenFSM(en_text_header, ScreenFanSelftest::get_inner_frame_rect()) {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
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
