/// @file
#include <gui/screen/initial/screen_welcome.hpp>

#include <lang/i18n.h>
#include <option/has_indx.h>
#include <printers.h>
#include <window_msgbox.hpp>
#include <option/has_xl_can.h>
#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
#endif

ScreenWelcome::ScreenWelcome()
    : PseudoScreenCallback {
        [] {
            const char *printer =
#if PRINTER_IS_PRUSA_XL()
                buddy::puppies::xl_can.is_enabled() ? "Prusa XL+" : "Original Prusa XL";
#elif PRINTER_IS_PRUSA_MK4()
                // The model is left out intentionally - it could be MK4, MK4S or MK3.9, we don't know yet
                "Original Prusa";
#elif PRINTER_IS_PRUSA_MK3_5()
                "Original Prusa MK3.5";
#elif PRINTER_IS_PRUSA_MINI()
                "Original Prusa MINI";
#elif PRINTER_IS_PRUSA_iX()
                "Original Prusa iX";
#elif PRINTER_IS_PRUSA_COREONE()
                HAS_INDX() ? "Prusa CORE One INDX" : "Prusa CORE One";
#elif PRINTER_IS_PRUSA_COREONEL()
                HAS_INDX() ? "Prusa CORE One L INDX" : "Prusa CORE One L";
#else
    #error
#endif
            static constexpr auto text = N_("Hi, this is your\n%s printer.\n"
                                            "I would like to guide you\nthrough the setup process.");
            StringViewUtf8Parameters<32> params;
            const string_view_utf8 greeting = _(text).formatted(params, printer);
            MsgBoxPepaCentered(greeting, Responses_Ok);
        },
    } {
}
