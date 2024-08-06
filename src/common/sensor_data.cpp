#include <common/sensor_data.hpp>

SensorData &sensor_data() {
    static SensorData instance;
    return instance;
}
