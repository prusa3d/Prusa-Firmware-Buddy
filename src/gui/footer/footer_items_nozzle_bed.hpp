/**
 * @file footer_items_nozzle_bed.hpp
 * @brief nozzle and bed heater items for footer
 */

#pragma once
#include "footer_items_heaters.hpp"
#include "config_features.h"
#include <option/has_toolchanger.h>

class FooterItemNozzle final : public FooterItemHeater {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemNozzle(window_t *parent);
};

class FooterItemNozzleDiameter final : public FooterIconText_FloatVal {
    static string_view_utf8 static_makeView(float value);
    static float static_readValue();

public:
    FooterItemNozzleDiameter(window_t *parent);
};

class FooterItemNozzlePWM final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemNozzlePWM(window_t *parent);
};

class FooterItemBed final : public FooterItemHeater {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();

public:
    FooterItemBed(window_t *parent);

protected:
#if ENABLED(MODULAR_HEATBED)
    uint16_t last_enabled_bedlet_mask { 0 };
    uint16_t last_warm_bedlet_mask { 0 };
#endif
    void unconditionalDraw() override;
    changed_t updateValue() override;

    static constexpr uint COLD = 40;
};

/**
 * @brief Show all temperatures, cycle all nozzles.
 */
class FooterItemAllNozzles final : public FooterIconText_IntVal {
    static string_view_utf8 static_makeView(int value);
    static int static_readValue();
    static footer::ItemDrawType GetDrawType();

    void unconditionalDraw() override;
    changed_t updateValue() override;

    static uint nozzle_n; ///< Cycle through nozzles, 0 is "Tool 1" displayed as "T1"
    static constexpr uint32_t CYCLE_TIME = 2000; ///< Time to cycle nozzles [ms]
    static constexpr uint COLD = 45; ///< Nozzle is cold under this [deg C]

#if HAS_TOOLCHANGER()
    static constexpr uint NOZZLES_COUNT = 5; ///< This icon only works for 5 nozzles
    static_assert(NOZZLES_COUNT <= EXTRUDERS);
#endif /*HAS_TOOLCHANGER()*/

public:
    FooterItemAllNozzles(window_t *parent);
};
