#include "leds/led_manager.hpp"
#include "display.hpp"
#include "led_lcd_cs_selector.hpp"

#include <leds/status_leds_handler.hpp>
#include <marlin_vars.hpp>
#include "neopixel.hpp"
#include <option/has_side_leds.h>
#include <device/peripherals.hpp>

#include <config_store/store_instance.hpp>

#if HAS_SIDE_LEDS()
    #include "leds/side_strip_handler.hpp"
#endif

#include <option/has_door_sensor.h>
#if HAS_DOOR_SENSOR()
    #include <buddy/door_sensor.hpp>
#endif

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif
#include <option/xbuddy_extension_variant.h>

#if XBUDDY_EXTENSION_VARIANT_IS_iX()
    #include "leds/afs_design_strip_handler.hpp"
#endif

#include <option/has_ac_controller.h>
#if HAS_AC_CONTROLLER()
    #include <leds/ac_controller_leds_handler.hpp>
#endif

#include <option/xl_enclosure_support.h>
#if XL_ENCLOSURE_SUPPORT()
    #include <CFanCtlEnclosure.hpp>
    #include <fanctl.hpp>
#endif

#include <option/has_indx.h>
#if HAS_INDX()
    #include <feature/indx_head_leds/indx_head_leds.hpp>
#endif

extern osThreadId displayTaskHandle;

using StatusLeds = neopixel::LedsSPI10M5Hz<4, GuiLedsWriter::write>;

static StatusLeds &get_status_leds() {
    static StatusLeds ret;
    return ret;
}

#if HAS_SIDE_LEDS()
    #if defined(UNITTESTS)
        // #error dead code found by automatic analyses (see BFW-5461)
        #define HAS_SIDE_LED_DRIVER() 0
    #elif PRINTER_IS_PRUSA_XL()
        #define HAS_SIDE_LED_DRIVER() 1
static constexpr size_t side_led_driver_count = 2;
    #elif PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
        #define HAS_SIDE_LED_DRIVER() 0
    #else
        #error "Not defined for this printer."
    #endif

    #if HAS_SIDE_LED_DRIVER()
using SideLeds = neopixel::LedsSPI10M5Hz<side_led_driver_count, SideStripWriter::write>;

static SideLeds &get_side_leds() {
    static SideLeds ret;
    return ret;
}
    #endif // HAS_SIDE_LED_DRIVER
#else
    #define HAS_SIDE_LED_DRIVER() 0
#endif // HAS_SIDE_LEDS

namespace leds {

LEDManager &LEDManager::instance() {
    static LEDManager instance;
    return instance;
}

void LEDManager::init() {
    // update the LEDs in init to turn them off (in case they were set to a color before a reset)
    // except the LCD backlight, set that to 100% brightness
    set_lcd_brightness(100);
    get_status_leds().update();
#if HAS_SIDE_LED_DRIVER()
    get_side_leds().update();
#endif
}

void LEDManager::update() {
    const uint32_t now = freertos::millis();
    if (!rate_limiter.check(now)) {
        return;
    }

    std::lock_guard lock(power_panic_mutex);
    if (power_panic) {
        return;
    }

    auto &status_leds_handler = leds::StatusLedsHandler::instance();
    status_leds_handler.update();
    auto data = status_leds_handler.led_data();

    auto &status_leds = get_status_leds();
    for (uint8_t i = 0; i < data.size(); ++i) {
        status_leds.set(data[i].data, i);
    }

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    // Bed LEDs copy LCD status bar strip
    buddy::xbuddy_extension().set_bed_leds_color(data[1].data);
#elif XBUDDY_EXTENSION_VARIANT_IS_iX()
    // Colored leds show the status LEDs color, unanimated
    auto &design_strip_handler = AFSDesignStripHandler::instance();
    design_strip_handler.update();
    buddy::xbuddy_extension().set_rgbw_led(design_strip_handler.color().data);

    // The white led is always fully on
    buddy::xbuddy_extension().set_white_led(255);
#endif

#if HAS_AC_CONTROLLER()
    // If we are printing set status as progress, otherwise static color
    auto color = ColorRGBW(data[1].r, data[1].g, data[1].b, data[1].w);
    leds::AcControllerLedsHandler::update(color, marlin_vars().sd_percent_done);
#endif

    status_leds.update();

#if HAS_SIDE_LEDS()
    auto &side_strip_handler = SideStripHandler::instance();
    #if HAS_DOOR_SENSOR()
    if (buddy::door_sensor().state() == buddy::DoorSensor::State::door_open) {
        side_strip_handler.activity_ping();
    }
    #endif

    side_strip_handler.update();

    #if HAS_SIDE_LED_DRIVER()
    auto color = side_strip_handler.color();
    auto &side_leds = get_side_leds();
    if constexpr (SideStripHandler::has_white_led_and_enclosure_on_second_driver()) {
        uint8_t second_led_green = 0;
        #if XL_ENCLOSURE_SUPPORT()
        second_led_green = static_cast<CFanCtlEnclosure &>(Fans::enclosure()).output_pwm();
        #endif

        // On XL, there are two neopixel drivers, the first one controls RGB of
        // the RGBW LED strip. The second one controls the white color of the
        // RGBW LED strip on its Green channel and the XL enclosure fan on its
        // Red channel.
        //
        // The channels on these strips are also apparently being GRB, not RGB,
        // as the Neopixel driver expects.
        side_leds.set(ColorRGBW(color.g, color.r, color.b).data, 0);
        side_leds.set(ColorRGBW(second_led_green, color.w, 0).data, 1);
    } else {
        for (uint8_t i = 0; i < side_led_driver_count; ++i) {
            side_leds.set(color.data, i);
        }
    }

    side_leds.update();
    #endif // HAS_SIDE_LED_DRIVER
#endif // HAS_SIDE_LEDS

#if HAS_INDX()
    // INDX head LEDs express the printer state (idle / busy / heating / warning).
    indx_head_leds::update();
#endif
}

void LEDManager::enter_power_panic() {
    {
        std::lock_guard lock(power_panic_mutex);
        power_panic = true;
    }

    // normally, GUI is accessing LCD & LEDs SPIs, but this is called from the
    // task handling power panic, and we need to turn the leds off quickly. So
    // we'll steal the display's SPI in a hacky way.

    // Temporary suspend display task, so that it doesn't interfere with turning off leds
    osThreadSuspend(displayTaskHandle);

    // Safe mode for display SPI is enabled (that disables DMA transfers and writes directly to SPI)
    display::enable_safe_mode();

    // Reinitialize SPI, so that we terminate any ongoing transfers to display or leds
#if BOARD_IS_XLBUDDY()
    hw_init_spi_side_leds();
#endif

    spi_init_lcd();
    auto &status_leds = get_status_leds();
    status_leds.set_all(0x0);
    status_leds.update();

#if HAS_SIDE_LED_DRIVER()
    auto &side_leds = get_side_leds();
    side_leds.set_all(0x0);
    side_leds.update();
#endif

    // reenable display task
    osThreadResume(displayTaskHandle);
}

void LEDManager::set_lcd_brightness(uint8_t brightness) {
    // LCD backlight is connected to the green channel of the fourth LED (index 3) on the status strip
    get_status_leds().set(ColorRGBW(0, (std::min<uint8_t>(brightness, 100) * 255) / 100, 0).data, 3);
}

} // namespace leds
