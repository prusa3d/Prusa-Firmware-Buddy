/**
 * @file IPause.hpp
 * @author Radek Vana
 * @brief basic interface of pause
 * @date 2021-01-08
 */

#pragma once

class IPause {
public:
    virtual void RestoreTemp() = 0;
    virtual bool CanSafetyTimerExpire() const = 0;
    virtual void NotifyExpiredFromSafetyTimer(float hotend_temp, float bed_temp) = 0;
    virtual bool HasTempToRestore() const = 0;
};
