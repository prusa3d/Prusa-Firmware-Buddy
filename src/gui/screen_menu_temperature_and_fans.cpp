/// @file
#include <screen_menu_temperature_and_fans.hpp>

#include <common/marlin_client.hpp>
#include <img_resources.hpp>
#include <printers.h>

#include <option/has_chamber_api.h>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

ScreenMenuTemperatureAndFans::ScreenMenuTemperatureAndFans()
    : ScreenMenuTemperatureAndFansBase {
        _("TEMPERATURE & FANS"),
    } {
    EnableLongHoldScreenAction();

#if (!PRINTER_IS_PRUSA_MINI())
    header.SetIcon(&img::temperature_white_16x16);
#endif // PRINTER_IS_PRUSA_MINI()

    Item<MI_TEMPERATURE_AND_FANS_COOLDOWN>().set_is_hidden(marlin_client::is_printing());
    Item<MI_TEMPERATURE_AND_FANS_COOLDOWN>().callback = [this] {
        for (auto tool : PhysicalToolIndex::all()) {
            marlin_client::set_target_nozzle(0, tool);
        }
        marlin_client::set_target_bed(0);
        marlin_client::set_fan_speed(0);
        Item<MI_HEATBED>().set_value(0);
        Item<MI_PRINTFAN>().set_value(0);
#if HAS_CHAMBER_API()
        if (buddy::chamber().capabilities().heating) {
            Item<MI_CHAMBER_TARGET_TEMP>().set_value(0);
        }
#endif
    };
}
