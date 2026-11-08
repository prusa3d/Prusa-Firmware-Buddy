#include "usb_host/usb_storage_gate.hpp"

#include <catch2/catch_test_macros.hpp>
#include <mutex>

namespace {

struct CountingLock {
    int depth = 0;
    int lock_calls = 0;
    int unlock_calls = 0;

    void lock() {
        ++lock_calls;
        ++depth;
    }

    void unlock() {
        ++unlock_calls;
        --depth;
    }
};

} // namespace

TEST_CASE("UsbStorageGate async hold is taken once for pipelined writes") {
    CountingLock exclusive;
    UsbStorageGate<CountingLock, std::mutex> gate { exclusive };

    gate.async_begin();
    REQUIRE(exclusive.depth == 1);
    REQUIRE(exclusive.lock_calls == 1);

    gate.async_begin();
    REQUIRE(exclusive.depth == 1);
    REQUIRE(exclusive.lock_calls == 1);

    gate.async_end();
    REQUIRE(exclusive.depth == 1);
    REQUIRE(exclusive.unlock_calls == 0);

    gate.async_end();
    REQUIRE(exclusive.depth == 0);
    REQUIRE(exclusive.unlock_calls == 1);
}

TEST_CASE("UsbStorageGate FatFs lock is a separate exclusive acquisition") {
    CountingLock exclusive;
    UsbStorageGate<CountingLock, std::mutex> gate { exclusive };

    gate.lock();
    REQUIRE(exclusive.depth == 1);
    gate.unlock();
    REQUIRE(exclusive.depth == 0);

    gate.async_begin();
    REQUIRE(exclusive.depth == 1);
    gate.async_end();
    REQUIRE(exclusive.depth == 0);
    REQUIRE(exclusive.lock_calls == 2);
    REQUIRE(exclusive.unlock_calls == 2);
}

TEST_CASE("UsbStorageGate extra async_end is ignored") {
    CountingLock exclusive;
    UsbStorageGate<CountingLock, std::mutex> gate { exclusive };

    gate.async_end();
    REQUIRE(exclusive.depth == 0);
    REQUIRE(exclusive.unlock_calls == 0);
}
