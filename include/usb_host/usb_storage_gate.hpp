#pragma once

#include <cstdint>

/// Exclusive USB volume access for FatFs disk I/O vs raw MSC writes.
///
/// FatFs `USBH_read` / `USBH_write` / `CTRL_SYNC` call lock()/unlock() on the
/// same task around a blocking MSC request.
///
/// Connect/PrusaLink uploads (`UsbPartialFile`) call async_begin() before
/// queueing a raw sector write and async_end() from the completion callback
/// (which may run on the USB MSC worker). The exclusive token stays held for
/// as long as at least one raw write is in flight, so FatFs metadata updates
/// (unlink, FAT, directory) cannot interleave with them.
///
/// ExclusiveLock::unlock() must be safe from a different task than lock()
/// (binary semaphore, not a FreeRTOS mutex).
///
/// Invariant: async_end() is only called for a successful async_begin(), and
/// not from a callback of a write that has not yet begun. That means
/// async_begin() may wait for ExclusiveLock while holding HoldMutex — there
/// is no in-flight callback that would need HoldMutex to release ExclusiveLock.
template <typename ExclusiveLock, typename HoldMutex>
class UsbStorageGate {
public:
    explicit UsbStorageGate(ExclusiveLock &exclusive)
        : exclusive_(exclusive) {}

    void lock() { exclusive_.lock(); }
    void unlock() { exclusive_.unlock(); }

    void async_begin() {
        hold_mtx_.lock();
        const bool take_exclusive = (async_holders_++ == 0);
        if (take_exclusive) {
            exclusive_.lock();
        }
        hold_mtx_.unlock();
    }

    void async_end() {
        hold_mtx_.lock();
        if (async_holders_ == 0) {
            hold_mtx_.unlock();
            return;
        }
        const bool give_exclusive = (--async_holders_ == 0);
        hold_mtx_.unlock();
        if (give_exclusive) {
            exclusive_.unlock();
        }
    }

private:
    ExclusiveLock &exclusive_;
    HoldMutex hold_mtx_ {};
    uint32_t async_holders_ = 0;
};
