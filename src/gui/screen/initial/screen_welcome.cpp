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
            const char *txt =
#if PRINTER_IS_PRUSA_XL()
                buddy::puppies::xl_can.is_enabled() ? N_("Hi, this is your\nPrusa XL+ printer.\n"
                                                         "I would like to guide you\nthrough the setup process.")
                                                    : N_("Hi, this is your\nOriginal Prusa XL printer.\n"
                                                         "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MK4()
                // The MK4 is left out intentionally - it could be MK4, MK4S or MK3.9, we don't know yet
                N_("Hi, this is your\nOriginal Prusa printer.\n"
                   "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MK3_5()
                N_("Hi, this is your\nOriginal Prusa MK3.5 printer.\n"
                   "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_MINI()
                N_("Hi, this is your\nOriginal Prusa MINI printer.\n"
                   "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_iX()
                N_("Hi, this is your\nOriginal Prusa iX printer.\n"
                   "I would like to guide you\nthrough the setup process.");
#elif PRINTER_IS_PRUSA_COREONE()
    #if HAS_INDX()
                N_("Hi, this is your\nPrusa CORE One INDX printer.\n"
                   "I would like to guide you\nthrough the setup process.");
    #else
                N_("Hi, this is your\nPrusa CORE One printer.\n"
                   "I would like to guide you\nthrough the setup process.");
    #endif
#elif PRINTER_IS_PRUSA_COREONEL()
    #if HAS_INDX()
                N_("Hi, this is your\nPrusa CORE One L INDX printer.\n"
                   "I would like to guide you\nthrough the setup process.");
    #else
                N_("Hi, this is your\nPrusa CORE One L printer.\n"
                   "I would like to guide you\nthrough the setup process.");
    #endif
#else
    #error
#endif
            MsgBoxPepaCentered(_(txt), Responses_Ok);
        },
    } {
}
