#include <freertos/mutex.hpp>
#include <mutex>

#include <option/filament_sensor.h>
#include <option/has_mmu2.h>
#include <option/has_xbuddy_extension.h>

#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <config_store/store_definition.hpp>
#include <bsod/bsod.h>

#if FILAMENT_SENSOR_IS_ADC()
    #include "filament_sensor_adc.hpp"
    #include "filament_sensor_adc_eval.hpp"
    #include "filters/median_filter.hpp"

#elif FILAMENT_SENSOR_IS_BINARY()
    #include "filament_sensor_photoelectric.hpp"

#else
    #error
#endif

#if HAS_MMU2()
    #include "filament_sensor_mmu.hpp"
#endif

#if HAS_XBUDDY_EXTENSION()
    #include "filament_sensor_xbuddy_extension.hpp"
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
#endif

static auto *extruder_filament_sensor(uint8_t index) {
#if FILAMENT_SENSOR_IS_ADC()
    static FSensorADC extruder_filament_sensor(FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 0 });
#elif FILAMENT_SENSOR_IS_BINARY()
    static FSensorPhotoElectric extruder_filament_sensor(FilamentSensorID { .position = FilamentSensorID::Position::extruder, .index = 0 });
#else
    #error
#endif

    return index == 0 ? &extruder_filament_sensor : nullptr;
}

// function returning abstract sensor - used in higher level api
IFSensor *GetExtruderFSensor(uint8_t index) {
    return extruder_filament_sensor(index);
}

IFSensor *GetSideFSensor([[maybe_unused]] uint8_t index) {
#if HAS_MMU2()
    if (index == 0 && config_store().mmu2_enabled.get()) {
        static FSensorMMU mmu_filament_sensor(FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 0 });
        return &mmu_filament_sensor;
    }
#endif

#if HAS_XBUDDY_EXTENSION()
    if (index == 0 && buddy::xbuddy_extension().status() != buddy::XBuddyExtension::Status::disabled) {
        static FSensorXBuddyExtension xbe_filament_sensor(FilamentSensorID { .position = FilamentSensorID::Position::side, .index = 0 }, FSensorXBuddyExtension::Source::gpio);
        return &xbe_filament_sensor;
    }
#endif

    return nullptr;
}

#if FILAMENT_SENSOR_IS_ADC()
// IRQ - called from interruption
void fs_process_sample(int32_t fs_raw_value, uint8_t tool_index) {
    static MedianFilter filter;

    FSensorADC *sensor = extruder_filament_sensor(tool_index);
    debug_assert(sensor);

    sensor->set_filtered_value_from_IRQ(filter.filter(fs_raw_value) ? fs_raw_value : FSensorADCEval::filtered_value_not_ready);
}
#endif
