#pragma once

#include <array>
#include <cstdint>
#include <algorithm>

class MotorStallFilter {
public:
    constexpr MotorStallFilter() = default;
    ~MotorStallFilter() = default;

    bool ProcessSample(int32_t value);

    void SetDetectionThreshold(float dt) {
        detectionThreshold = dt;
    }

    constexpr float DetectionThreshold() const { return detectionThreshold; }

#ifndef UNITTEST
private:
#endif
    std::array<int32_t, 5> buffer {}; // Buffer to store previous input values
    float detectionThreshold = 700'000.F;
#ifdef UNITTEST
    float filteredValue; // for unit tests, filteredValue is a member of the class allowing better debugging experience
#endif
};

class EMotorStallDetector {
    friend class BlockEStallDetection;
    friend class EStallDetectionStateLatch;

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

    /// @returns raw detected flag disregarding blocked and enabled flags
    constexpr bool DetectedRaw() const { return detected; }

    /// @returns true if a stall has been detected somewhere in the past.
    /// The idea is to keep the filter running almost asynchronnously in the back and ask for the @ref detected flag occasionally.
    /// When the flag has been read by the caller code, it may clear it in order to get a new detected == true in the future.
    constexpr bool DetectedUnreported() const {
        return detected && (!reported) && (blocked <= 0) && enabled;
    }

    inline void ClearDetected() {
        detected = false;
        reported = false;
    }

    inline bool Reported() const {
        return reported;
    }

    inline void ClearReported() {
        reported = false;
    }

    constexpr bool Enabled() const {
        return enabled;
    }

    void SetEnabled(bool set = true);

    /// Performs evaluation of various runtime conditions.
    /// When a stall is detected, sets \p reported to true (even if not enabled)
    /// @returns true when the preceding sequence of load cell data events is considered as a stalled E-motor.
    bool Evaluate(bool movingE, bool directionE);

    void SetDetectionThreshold(float dt) {
        emf.SetDetectionThreshold(dt);
    }
    constexpr float DetectionThreshold() const { return emf.DetectionThreshold(); }

#ifndef UNITTEST
private:
#endif
    constexpr EMotorStallDetector() {}
    ~EMotorStallDetector() = default;

    MotorStallFilter emf;

    /// Used to prevent the filter from generating further detected events even if they were real - e.g. while doing a toolchange.
    /// Increased/decreased by each blocker. >0 = blocked. <0 should never happen
    int blocked = 0;

    /// true if filter value exceeded the threshold
    bool detected : 1 = false;

    /// Set to true when \ref Evaluate returns true.
    bool reported : 1 = false;

    /// Enables/Disables the whole feature - used to control the filter from the UI
    /// Disabled by default, the outer code must enable the filter first (probably after reading a flag from EEPROM)
    bool enabled : 1 = false;
};

class BlockEStallDetection {
public:
    BlockEStallDetection() {
        auto &emsd = EMotorStallDetector::Instance();
        emsd.blocked++;
        emsd.detected = false;
    }
    ~BlockEStallDetection() {
        auto &emsd = EMotorStallDetector::Instance();
        emsd.blocked--;
    }
};

class EStallDetectionStateLatch {
    float detectionThreshold;
    bool enabled;

public:
    EStallDetectionStateLatch() {
        auto &emsd = EMotorStallDetector::Instance();
        enabled = emsd.enabled;
        detectionThreshold = emsd.DetectionThreshold();
    }
    ~EStallDetectionStateLatch() {
        auto &emsd = EMotorStallDetector::Instance();
        emsd.SetEnabled(enabled);
        emsd.ClearDetected(); // disregard everything which happened during the state latch being active
        emsd.SetDetectionThreshold(detectionThreshold);
    }
};
