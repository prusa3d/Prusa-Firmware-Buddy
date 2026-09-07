/// @file
#include <gui/screen/initial/screen_print_readiness.hpp>

#include <config_store/store_instance.hpp>
#include <lang/i18n.h>
#include <window_msgbox.hpp>

#include <option/has_selftest.h>
#if HAS_SELFTEST()
    #include <marlin_client.hpp>
    #include <selftest_result_evaluation.hpp>
#endif

bool ScreenPrintReadiness::should_show() {
#if HAS_SELFTEST()
    if (!is_selftest_successfully_completed()) {
        return true;
    }
#endif
    return !config_store().happy_printing_seen.get();
}

ScreenPrintReadiness::ScreenPrintReadiness()
    : PseudoScreenCallback {
        [] {
#if HAS_SELFTEST()
            if (!is_selftest_successfully_completed()) {
                marlin_client::set_warning(WarningType::SelftestNotSuccessfullyCompleted);
                return;
            }
#endif
            if (!config_store().happy_printing_seen.get()) {
                MsgBoxPepaCentered(_("Happy printing!"), { Response::Continue, Response::_none, Response::_none, Response::_none });
                config_store().happy_printing_seen.set(true);
            }
        },
    } {
}
