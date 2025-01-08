/**
 * @file footer_item_fsensor.hpp
 * @brief footer item displaying filament sensor state
 */

#pragma once
#include "ifooter_item.hpp"
#include "filament.hpp"

class FooterItemFSensor final : public FooterIconText_IntVal {
    static int static_readValue();

public:
    static string_view_utf8 static_makeView(int value);
    FooterItemFSensor(window_t *parent);
};

/**
 * @brief Variant of filament sensor for side sensor.
 */
class FooterItemFSensorSide final : public FooterIconText_IntVal {

    /**
     * @brief Reuse makeView from tool filament sensor.
     * @param value FilamentSensorState state converted to int
     */
    static inline string_view_utf8 static_makeView(int value) {
        return FooterItemFSensor::static_makeView(value);
    }

    /**
     * @brief Get side filament sensor value converted to int.
     * To be used by FooterItemFSensor::static_makeView().
     */
    static int static_readValue();

public:
    /**
     * @brief Construct side filament sensor footer.
     * @param parent uncommented GUI stuff
     */
    FooterItemFSensorSide(window_t *parent);
};
