#pragma once

#include <array>
#include <cstdint>
#include <algorithm>

class MotorStallFilter {
public:
    constexpr MotorStallFilter() = default;
    ~MotorStallFilter() = default;

    bool ProcessSample(int32_t value);

#ifndef UNITTEST
private:
#endif
    std::array<int32_t, 5> buffer {}; // Buffer to store previous input values
    static constexpr float detectionThreshold = 700'000.F;
#ifdef UNITTEST
    float filteredValue; // for unit tests, filteredValue is a member of the class allowing better debugging experience
#endif
};

class EMotorStallDetector {
public:
    /// Singleton instance of the E-motor stall detector
    /// Technically, it is not required to have exactly one instance of this class in the firmware,
    /// but it makes it simpler to use.
    static EMotorStallDetector &Instance() {
        static EMotorStallDetector emsd;
        return emsd;
    }

    /// Record and process incoming raw readings from the LoadCell
    void ProcessSample(int32_t value);

    /// @returns true if a stall has been detected somewhere in the past.
    /// The idea is to keep the filter running almost asynchronnously in the back and ask for the @ref detected flag occasionally.
    /// When the flag has been read by the caller code, it may clear it in order to get a new detected == true in the future.
    constexpr bool Detected() const {
        return detected && (!blocked) && enabled;
    }
    void ClearDetected() {
        detected = false;
    }

    constexpr bool Blocked() const {
        return blocked;
    }
    /// If blocked, @ref Detected always returns false.
    void Block() {
        blocked = true;
    }
    void Unblock() {
        blocked = false;
        detected = false;
    }

    constexpr bool Enabled() const {
        return enabled;
    }
    void Enable() {
        enabled = true;
    }
    void Disable() {
        enabled = false;
        detected = false;
        blocked = false;
    }

    /// Performs evaluation of various runtime conditions.
    /// @returns true when the preceding sequence of load cell data events is considered as a stalled E-motor
    bool Evaluate(bool movingE, bool directionE);

#ifndef UNITTEST
private:
#endif
    constexpr EMotorStallDetector()
        : detected(false)
        , blocked(false)
        , enabled(false) // disabled by default, the outer code must enable the filter first (probably after reading a flag from EEPROM)
    {}
    ~EMotorStallDetector() = default;

    MotorStallFilter emf;
    bool detected; ///< true if filter value exceeded the threshold
    bool blocked; ///< The block flag is used to prevent the filter from generating further detected events even if they were real - e.g. while doing a toolchange.
    bool enabled; ///< Enables/Disables the whole feature - used to control the filter from the UI
};

class BlockEStallDetection {
public:
    BlockEStallDetection() {
        EMotorStallDetector::Instance().Block();
    }
    ~BlockEStallDetection() {
        EMotorStallDetector::Instance().Unblock();
    }
};
