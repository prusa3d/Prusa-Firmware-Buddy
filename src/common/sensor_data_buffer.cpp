#include "sensor_data_buffer.hpp"

static SemaphoreHandle_t mutex = nullptr;
static SensorData::SensorDataBuffer *buffer = nullptr;

void info_screen_handler(metric_point_t *point) {
    SensorData::HandleNewData(point);
}

using namespace SensorData;

SensorDataBuffer::SensorDataBuffer() {
    mutex = xSemaphoreCreateMutex();
    RegisterBuffer(this);
    enableMetrics();
}

SensorDataBuffer::~SensorDataBuffer() {
    UnregisterBuffer();
    disableMetrics();
}

metric_handler_t *SensorDataBuffer::getHandler() {

    for (metric_handler_t **handlers = metric_get_handlers(); *handlers != nullptr; handlers++) {
        metric_handler_t *handler = *handlers;
        if (strcmp(handler->name, "SENSOR_INFO_SCREEN") == 0) {
            return handler;
        }
    }
    return nullptr;
}

#if BOARD_IS_XLBUDDY
static constexpr Sensor first_sensor_to_log = Sensor::bedTemp;
#elif BOARD_IS_XBUDDY
static constexpr Sensor first_sensor_to_log = Sensor::printFan;
#else
static constexpr Sensor first_sensor_to_log = Sensor::MCUTemp;
#endif
bool SensorDataBuffer::enableMetrics() {

    if (allMetricsEnabled) {
        return true;
    }
    size_t count = 0;
    metric_t *metric = metric_get_linked_list();
    metric_handler_t *handler = getHandler();
    if (!handler) {
        return false;
    }
    // step through all metrics and enable the handler for metrics which we want to display
    while (metric) {
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
        metric = metric->next;
    }
    return false;
}

void SensorDataBuffer::disableMetrics() {
    metric_t *metric = metric_get_linked_list();
    metric_handler_t *handler = getHandler();
    if (!handler) {
        return;
    }
    // step through all metrics and disable the handler for metrics which we want to display
    while (metric) {
        auto it = std::lower_bound(sensors.begin(), sensors.end(), pair { metric->name, first_sensor_to_log }, compareFN {});
        if (it != sensors.end() && strcmp(metric->name, it->first) == 0) {
            metric->enabled_handlers &= ~(1 << handler->identifier);
            sensorValues[static_cast<size_t>(it->second)] = Value();
        }
        metric = metric->next;
    }
    allMetricsEnabled = false;
}

Value SensorDataBuffer::GetValue(Sensor type) {

    Value val;
    enableMetrics();

    if (xSemaphoreTake(mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
        val = sensorValues[static_cast<size_t>(type)];
    };
    xSemaphoreGive(mutex);
    return val;
}

void SensorDataBuffer::HandleNewData(metric_point_t *point) {
    // check if metric value is int or float, because we want only these
    if (point->metric->type == METRIC_VALUE_FLOAT || point->metric->type == METRIC_VALUE_INTEGER) {
        auto it = std::lower_bound(sensors.begin(), sensors.end(), pair { point->metric->name, first_sensor_to_log }, compareFN {});
        if (it != sensors.end() && strcmp(it->first, point->metric->name) == 0) {
            if (xSemaphoreTake(mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
                if (point->metric->type == METRIC_VALUE_FLOAT) {
                    sensorValues[static_cast<uint16_t>(it->second)] = Value(point->value_float);
                } else {
                    sensorValues[static_cast<uint16_t>(it->second)] = Value(point->value_int);
                }
                xSemaphoreGive(mutex);
            }
        }
    }
}

void initMutex() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
    }
}

void SensorData::RegisterBuffer(SensorDataBuffer *buff) {
    assert(buffer == nullptr); // some other SensorData is already registered
    initMutex();
    if (xSemaphoreTake(mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
        buffer = buff;
        xSemaphoreGive(mutex);
    }
}

void SensorData::UnregisterBuffer() {
    initMutex();
    if (xSemaphoreTake(mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
        buffer = nullptr;
        xSemaphoreGive(mutex);
    }
}

void SensorData::HandleNewData(metric_point_t *point) {
    initMutex();
    if (xSemaphoreTake(mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
        if (buffer != nullptr) {
            buffer->HandleNewData(point);
        }
        xSemaphoreGive(mutex);
    }
}
