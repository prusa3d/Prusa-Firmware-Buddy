#include "sensor_data_buffer.hpp"

static freertos::Mutex mutex;
static SensorData::SensorDataBuffer *buffer = nullptr;

void info_screen_handler(metric_point_t *point) {
    SensorData::HandleNewData(point);
}

using namespace SensorData;

SensorDataBuffer::SensorDataBuffer() {
    RegisterBuffer(this);
    enableMetrics();
}

SensorDataBuffer::~SensorDataBuffer() {
    UnregisterBuffer();
    disableMetrics();
}

const metric_handler_t *SensorDataBuffer::getHandler() {
    for (auto handlers = metric_get_handlers(); *handlers; handlers++) {
        auto handler = *handlers;
        if (strcmp(handler->name, "SENSOR_INFO_SCREEN") == 0) {
            return handler;
        }
    }
    return nullptr;
}

#if BOARD_IS_XLBUDDY()
static constexpr Sensor first_sensor_to_log = Sensor::bedTemp;
#elif BOARD_IS_XBUDDY()
static constexpr Sensor first_sensor_to_log = Sensor::printFan;
#else
static constexpr Sensor first_sensor_to_log = Sensor::MCUTemp;
#endif
bool SensorDataBuffer::enableMetrics() {

    if (allMetricsEnabled) {
        return true;
    }
    size_t count = 0;
    const metric_handler_t *handler = getHandler();
    if (!handler) {
        return false;
    }
    // step through all metrics and enable the handler for metrics which we want to display
    for (auto metric = metric_get_iterator_begin(), e = metric_get_iterator_end(); metric != e; metric++) {
        auto it = std::lower_bound(sensors.begin(), sensors.end(), pair { metric->name, first_sensor_to_log }, compareFN {});
        if (it != sensors.end() && strcmp(metric->name, it->first) == 0) {
            count++;
            metric->enabled_handlers |= (1 << handler->identifier);
            sensorValues[static_cast<size_t>(it->second)].set_enabled();
        }
        // check if we enabled handler for all metrics, if yes return true
        if (count == sensors.size()) {
            allMetricsEnabled = true;
            return true;
        }
    }
    return false;
}

void SensorDataBuffer::disableMetrics() {
    const metric_handler_t *handler = getHandler();
    if (!handler) {
        return;
    }
    // step through all metrics and disable the handler for metrics which we want to display
    for (auto metric = metric_get_iterator_begin(), e = metric_get_iterator_end(); metric != e; metric++) {
        auto it = std::lower_bound(sensors.begin(), sensors.end(), pair { metric->name, first_sensor_to_log }, compareFN {});
        if (it != sensors.end() && strcmp(metric->name, it->first) == 0) {
            metric->enabled_handlers &= ~(1 << handler->identifier);
            sensorValues[static_cast<size_t>(it->second)] = Value();
        }
    }
    allMetricsEnabled = false;
}

Value SensorDataBuffer::GetValue(Sensor type) {
    enableMetrics();
    std::unique_lock lock { mutex };
    return sensorValues[static_cast<size_t>(type)];
}

void SensorDataBuffer::HandleNewData(metric_point_t *point) {
    // check if metric value is int or float, because we want only these
    if (point->metric->type == METRIC_VALUE_FLOAT || point->metric->type == METRIC_VALUE_INTEGER) {
        auto it = std::lower_bound(sensors.begin(), sensors.end(), pair { point->metric->name, first_sensor_to_log }, compareFN {});
        if (it != sensors.end() && strcmp(it->first, point->metric->name) == 0) {
            std::unique_lock lock { mutex };
            if (point->metric->type == METRIC_VALUE_FLOAT) {
                sensorValues[static_cast<uint16_t>(it->second)] = Value(point->value_float);
            } else {
                sensorValues[static_cast<uint16_t>(it->second)] = Value(point->value_int);
            }
        }
    }
}

void SensorData::RegisterBuffer(SensorDataBuffer *buff) {
    assert(buffer == nullptr); // some other SensorData is already registered
    std::unique_lock lock { mutex };
    buffer = buff;
}

void SensorData::UnregisterBuffer() {
    std::unique_lock lock { mutex };
    buffer = nullptr;
}

void SensorData::HandleNewData(metric_point_t *point) {
    std::unique_lock lock { mutex };
    if (buffer != nullptr) {
        buffer->HandleNewData(point);
    }
}
