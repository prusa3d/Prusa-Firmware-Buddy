#pragma once
#include "metric.h"
#include <array>
#include <algorithm>
#include <string.h>
#include <freertos/mutex.hpp>
#include "cmath_ext.h"
#include "ConstexprQuickSort.hpp"
#include <device/board.h>
namespace SensorData {

#if BOARD_IS_XLBUDDY()
enum class Sensor {
    bedTemp,
    boardTemp,
    MCUTemp,
    inputVoltage,
    sandwich5VVoltage,
    sandwich5VCurrent,
    buddy5VCurrent,
    loadCell,
    dwarfBoardTemperature,
    dwarfMCUTemperature,
    printFan,
    hbrFan,
    printFanAct,
    hbrFanAct,
    mbedMCUTemperature,
    count,
};
#elif BOARD_IS_XBUDDY()
enum class Sensor {
    bedTemp,
    boardTemp,
    MCUTemp,
    loadCell,
    printFan,
    hbrFan,
    printFanAct,
    hbrFanAct,
    heaterVoltage,
    inputVoltage,
    heaterCurrent,
    inputCurrent,
    mmuCurrent,
    count,
};
#else
enum class Sensor {
    MCUTemp,
    count,
};
#endif

template <typename T1, typename T2>
struct pair {

    T1 first;
    T2 second;

    constexpr pair(T1 first, T2 second)
        : first(first)
        , second(second) {}
};

struct compareFN {
    constexpr bool operator()(const pair<const char *, Sensor> &a, const pair<const char *, Sensor> &b) const {
        auto *p1 = a.first;
        auto *p2 = b.first;

        while (*p1 && *p1 == *p2) {
            ++p1, ++p2;
        }

        return ((*p1 > *p2) - (*p2 > *p1)) > 0;
    }
};
enum class Type : uint8_t {
    intType,
    floatType
};

class Value {
    union {
        int32_t int_val;
        float float_val;
    };

    struct {
        bool valid;
        bool enabled;
        Type type;
    } attribute;

public:
    /**
     * @brief Get the float value.
     * @note You need to check which type this is first.
     * @return float value
     */
    float get_float() {
        assert(attribute.type == Type::floatType);
        return float_val;
    }

    /**
     * @brief Get the int value.
     * @note You need to check which type this is first.
     * @return int value
     */
    int get_int() {
        assert(attribute.type == Type::intType);
        return int_val;
    }

    /// @return true if value is valid
    bool is_valid() const {
        return attribute.valid;
    }

    /// @brief Set value to enabled. Doesn't have to be valid.
    void set_enabled() {
        attribute.enabled = true;
    }

    /// @return true if value is enabled
    bool is_enabled() const {
        return attribute.enabled;
    }

    /// @return type of the value
    Type get_type() const {
        return attribute.type;
    }

    constexpr bool operator==(const Value &lhs) const {
        if (attribute.valid != lhs.attribute.valid) {
            return false;
        }
        if (attribute.enabled != lhs.attribute.enabled) {
            return false;
        }
        if (attribute.type != lhs.attribute.type) {
            return false;
        }
        switch (attribute.type) {
        case Type::intType:
            return int_val == lhs.int_val;
        case Type::floatType:
            return nearlyEqual(float_val, lhs.float_val, 0.1999f);
        }
        return false;
    }
    constexpr bool operator!=(const Value &lhs) const {
        return !(*this == lhs);
    }

    /// @brief Create invalid, disabled value.
    Value()
        : int_val(-1)
        , attribute { false, false, Type::intType } {}

    /// @brief Create valid float value.
    Value(float val)
        : float_val(val)
        , attribute { true, true, Type::floatType } {}

    /// @brief Create valid integer value.
    Value(int val)
        : int_val(val)
        , attribute { true, true, Type::intType } {}
};
static_assert(sizeof(Value) <= 8, "change attribute to bit field");

using SensorArray = std::array<pair<const char *, Sensor>, static_cast<size_t>(Sensor::count)>;

static_assert(sizeof(int32_t) == sizeof(float), "float is not 4 bytes large");

// this struct collects data from metrics and gives access to the
// sensor info screen
struct SensorDataBuffer {
private:
    /// gets pointer to the handler, which handles the data send to this structs
    /// \return pointer to the handler
    const metric_handler_t *getHandler();

    /// enables all metrics from sensors array
    ///  \return true if all metrics are enabled, false otherwise
    bool enableMetrics();
    /// disables all metrics from sensors array
    void disableMetrics();

    freertos::Mutex mutex;
    bool allMetricsEnabled = false;

    // array of metrics we want to show in sensor screen
#if BOARD_IS_XLBUDDY()
    constexpr static SensorArray sensors = ConstexprQuickSort::sort(SensorArray {
                                                                        {
                                                                            { "temp_bed", Sensor::bedTemp },
                                                                            { "temp_brd", Sensor::boardTemp },
                                                                            { "temp_mcu", Sensor::MCUTemp },
                                                                            { "24VVoltage", Sensor::inputVoltage },
                                                                            { "5VVoltage", Sensor::sandwich5VVoltage },
                                                                            { "Sandwitch5VCurrent", Sensor::sandwich5VCurrent },
                                                                            { "xlbuddy5VCurrent", Sensor::buddy5VCurrent },
                                                                            { "loadcell_value", Sensor::loadCell },
                                                                            { "dwarf_board_temp", Sensor::dwarfBoardTemperature },
                                                                            { "dwarf_mcu_temp", Sensor::dwarfMCUTemperature },
                                                                            { "fan_speed", Sensor::printFan },
                                                                            { "fan_hbr_speed", Sensor::hbrFan },
                                                                            { "print_fan_act", Sensor::printFanAct },
                                                                            { "hbr_fan_act", Sensor::hbrFanAct },
                                                                            { "bed_mcu_temp", Sensor::mbedMCUTemperature },
                                                                        },
                                                                    },
        compareFN {});

#elif BOARD_IS_XBUDDY()
    constexpr static SensorArray sensors = ConstexprQuickSort::sort(SensorArray {
                                                                        { { "temp_bed", Sensor::bedTemp },
                                                                            { "temp_brd", Sensor::boardTemp },
                                                                            { "temp_mcu", Sensor::MCUTemp },
                                                                            { "fan_speed", Sensor::printFan },
                                                                            { "loadcell_value", Sensor::loadCell },
                                                                            { "fan_hbr_speed", Sensor::hbrFan },
                                                                            { "print_fan_act", Sensor::printFanAct },
                                                                            { "hbr_fan_act", Sensor::hbrFanAct },
                                                                            { "volt_nozz", Sensor::heaterVoltage },
                                                                            { "volt_bed", Sensor::inputVoltage },
                                                                            { "curr_nozz", Sensor::heaterCurrent },
                                                                            { "curr_inp", Sensor::inputCurrent },
                                                                            { "cur_mmu_imp", Sensor::mmuCurrent } },
                                                                    },
        compareFN {});

#else
    constexpr static SensorArray sensors = ConstexprQuickSort::sort(SensorArray {
                                                                        {
                                                                            { "temp_mcu", Sensor::MCUTemp },
                                                                        },
                                                                    },
        compareFN {});

#endif

    // array for storing values of sensors
    std::array<Value, static_cast<uint16_t>(Sensor::count)>
        sensorValues {};

public:
    /// enables metric handler for chosen metrics and creates mutex
    SensorDataBuffer();
    SensorDataBuffer(SensorDataBuffer &other) = delete;
    SensorDataBuffer(SensorDataBuffer &&other) = delete;

    /// disables metric handler for chosen metrics
    ~SensorDataBuffer();

    /// Call this function to get value of metric
    ///  \param type which sensor value to return
    ///  \return union of int and flor and validity byte (currently is used only one bit from the byte)
    Value GetValue(Sensor type);

    /// function to be called from the metric_handler_t struct
    /// \param point
    void HandleNewData(metric_point_t *point);
};

/// Registers buffer
/// \param buff pointer to buffer
void RegisterBuffer(SensorDataBuffer *buff);

/// Unregisters the buffer
void UnregisterBuffer();

/// Handles incoming data from metrics
/// \param point to handle
void HandleNewData(metric_point_t *point);

} // namespace SensorData
