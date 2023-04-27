/**
 * @file IPause.hpp
 * @author Radek Vana
 * @brief basic interface of pause
 * @date 2021-01-08
 */

#pragma once

class IPause {
public:
    virtual ~IPause() = default;
    virtual void RestoreTemp() = 0;
    virtual bool CanSafetyTimerExpire() const = 0;
    virtual void NotifyExpiredFromSafetyTimer() = 0;
    virtual bool HasTempToRestore() const = 0;
};
