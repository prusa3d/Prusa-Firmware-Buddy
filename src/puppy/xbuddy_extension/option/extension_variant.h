#pragma once

#if defined(EXTENSION_VARIANT) && EXTENSION_VARIANT == EXTENSION_STANDARD
    #define EXTENSION_IS_STANDARD() 1
#elif defined(EXTENSION_VARIANT) && EXTENSION_VARIANT == EXTENSION_IX
    #define EXTENSION_IS_IX() 1
#elif defined(EXTENSION_VARIANT) && EXTENSION_VARIANT == EXTENSION_XL_CAN
    #define EXTENSION_IS_XL_CAN() 1
#else
    #error Please define the EXTENSION_VARIANT macro
#endif

#ifndef EXTENSION_IS_STANDARD
    #define EXTENSION_IS_STANDARD() 0
#endif

#ifndef EXTENSION_IS_IX
    #define EXTENSION_IS_IX() 0
#endif

#ifndef EXTENSION_IS_XL_CAN
    #define EXTENSION_IS_XL_CAN() 0
#endif

#if EXTENSION_IS_XL_CAN()
    // PA6 is the fan 5 V power gate on the bridge PCB (hal::fan_power)
    #define PA6_PIN_DRIVES_W_LED()     0
    #define PA6_PIN_DRIVES_FAN_POWER() 1

    #define HAS_GPIO_EXPANDER() 0
#else
    // PA6 is TIM3_CH1 driving white led
    #define PA6_PIN_DRIVES_W_LED()     1
    #define PA6_PIN_DRIVES_FAN_POWER() 0

    #define HAS_GPIO_EXPANDER() 1
#endif

static_assert(PA6_PIN_DRIVES_W_LED() + PA6_PIN_DRIVES_FAN_POWER() <= 1);
