#include <catch2/catch_test_macros.hpp>

#include <nfcv/field_manager.hpp>

#include <algorithm>
#include <deque>
#include <limits>
#include <variant>
#include <vector>

namespace {

/// Records what the manager asks of the reader. The command path is never exercised here -
/// FieldManager only ever raises and drops the field.
struct FakeReader : public nfcv::ReaderWriterInterface {
    struct Up {
        AntennaID antenna;
        bool operator==(const Up &) const = default;
    };
    struct Down {
        bool operator==(const Down &) const = default;
    };
    using Event = std::variant<Up, Down>;

    std::vector<Event> events;

    /// Errors the next try_field_up calls fail with, consumed front to back.
    /// An exhausted deque means every further attempt succeeds.
    std::deque<nfcv::Error> up_errors;

    /// Set to fail every attempt, for the cases that must run the arbitration out
    std::optional<nfcv::Error> always_fail_with;

    /// Every try_field_up, contended ones included
    unsigned up_attempts = 0;

    nfcv::Result<void> try_field_up(AntennaID antenna) final {
        up_attempts++;

        if (!up_errors.empty()) {
            const auto error = up_errors.front();
            up_errors.pop_front();
            return std::unexpected(error);
        }
        if (always_fail_with.has_value()) {
            return std::unexpected(*always_fail_with);
        }

        events.push_back(Up { .antenna = antenna });
        return {};
    }

    void field_down() final {
        events.push_back(Down {});
    }

    AntennaID antenna_count() const final {
        return 2;
    }

    nfcv::Result<void> nfcv_command(const nfcv::Command &) final {
        return std::unexpected(nfcv::Error::not_implemented);
    }

    size_t count_downs() const {
        return std::ranges::count_if(events, [](const Event &e) { return std::holds_alternative<Down>(e); });
    }
};

/// The platform hooks are plain function pointers, exactly like on hardware, so their state is
/// static and every \ref Fixture resets it.
struct FakeSystem {
    /// The manager reads the clock whenever it needs the time; the tests move it by hand
    uint32_t now_ms = 0;

    /// Every backoff the arbitration slept for, in order
    std::vector<uint32_t> delays;

    /// Values handed out as randomness, cycled through in order.
    /// Full scale - the manager scales the top bits, so small numbers all map to the minimum.
    std::vector<uint32_t> random_values { 0, std::numeric_limits<uint32_t>::max() / 2, std::numeric_limits<uint32_t>::max() };
    size_t next_random = 0;
};

FakeSystem fake;

uint32_t fake_now_ms() {
    return fake.now_ms;
}

/// Never really sleeps - it just moves the clock, which is what makes the arbitration deadline
/// reachable without the tests waiting for real time to pass
void fake_delay_ms(uint32_t delay_ms) {
    fake.delays.push_back(delay_ms);
    fake.now_ms += delay_ms;
}

uint32_t fake_random_u32() {
    const auto value = fake.random_values.at(fake.next_random % fake.random_values.size());
    fake.next_random++;
    return value;
}

constexpr nfcv::FieldManager::System fake_system {
    .now_ms = fake_now_ms,
    .delay_ms = fake_delay_ms,
    .random_u32 = fake_random_u32,
};

constexpr uint32_t linger_ms = nfcv::FieldManager::linger_ms;
constexpr uint32_t hold_max_ms = nfcv::FieldManager::hold_max_ms;

/// Arbitration windows the OPT backend uses: a host-requested operation waits out a neighbour's
/// whole hold, a background scan does not
constexpr uint32_t io_window_ms = 3000;
constexpr uint32_t discovery_window_ms = 500;

/// No retries at all, just the single collision-avoidance attempt
constexpr uint32_t single_attempt = 0;

using Freshness = nfcv::FieldManager::Freshness;

/// A manager over a fresh reader and a reset platform state
struct Fixture {
    FakeReader reader;
    nfcv::FieldManager manager { reader, fake_system };

    explicit Fixture(uint32_t now_ms = 0) {
        // The manager reads the clock only from acquire() onwards, so resetting it here - after
        // the members are constructed - is early enough
        fake = FakeSystem { .now_ms = now_ms };
    }

    /// An operation that hands the field back still radiating
    void lingering_op() {
        auto guard = manager.acquire(0, io_window_ms, Freshness::reuse);
        REQUIRE(guard);
        guard.keep_field_up();
    }
};

} // namespace

TEST_CASE("FieldManager guards own the field", "[nfcv][field_manager]") {
    Fixture f;

    SECTION("a plain release drops the field") {
        {
            auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
            REQUIRE(guard);
            CHECK(f.manager.is_field_up());
            CHECK(f.reader.count_downs() == 0);
        }

        CHECK(f.reader.count_downs() == 1);
        CHECK(!f.manager.is_field_up());
    }

    SECTION("keep_field_up leaves the field radiating past the release") {
        f.lingering_op();

        CHECK(f.reader.count_downs() == 0);
        CHECK(f.manager.is_field_up());
    }

    SECTION("the requested antenna and window reach the reader") {
        auto guard = f.manager.acquire(1, discovery_window_ms, Freshness::reuse);
        REQUIRE(guard);
        CHECK(f.reader.events == std::vector<FakeReader::Event> {
                  FakeReader::Up { .antenna = 1 },
              });
    }

    SECTION("a failed acquisition leaves nothing radiating and no bookkeeping") {
        f.reader.up_errors = { nfcv::Error::field_collision };

        {
            auto guard = f.manager.acquire(0, single_attempt, Freshness::reuse);
            CHECK(!guard);
            CHECK(guard.error() == nfcv::Error::field_collision);
            guard.keep_field_up();
        }

        // try_field_up leaves nothing radiating on failure, so the release must not add a drop
        CHECK(f.reader.count_downs() == 0);
        CHECK(!f.manager.is_field_up());

        fake.now_ms = linger_ms;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 0);
    }
}

TEST_CASE("FieldManager expires the linger", "[nfcv][field_manager]") {
    Fixture f;

    SECTION("a tick inside the window keeps the field, a tick at the edge drops it") {
        f.lingering_op();

        fake.now_ms = linger_ms - 1;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 0);
        CHECK(f.manager.is_field_up());

        fake.now_ms = linger_ms;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 1);
        CHECK(!f.manager.is_field_up());
    }

    SECTION("a second op restarts the linger") {
        f.lingering_op();

        fake.now_ms = 100;
        f.lingering_op();

        fake.now_ms = linger_ms;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 0);

        fake.now_ms = 100 + linger_ms;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 1);
    }

    SECTION("drop_field clears the linger, so no second drop follows") {
        f.lingering_op();

        f.manager.drop_field();
        CHECK(f.reader.count_downs() == 1);
        CHECK(!f.manager.is_field_up());

        fake.now_ms = linger_ms;
        f.manager.tick();
        CHECK(f.reader.count_downs() == 1);
    }

    SECTION("a tick with the field down does nothing") {
        fake.now_ms = linger_ms;
        f.manager.tick();
        CHECK(f.reader.events.empty());
    }
}

TEST_CASE("FieldManager caps one continuous hold", "[nfcv][field_manager]") {
    Fixture f;

    f.lingering_op();

    // Well inside the linger each time, so the burst never actually drops
    for (uint32_t time = linger_ms / 2; time < hold_max_ms; time += linger_ms / 2) {
        fake.now_ms = time;
        f.lingering_op();
        f.manager.tick();
        CHECK(f.manager.is_field_up());
    }

    CHECK(f.reader.count_downs() == 0);

    fake.now_ms = hold_max_ms;
    f.lingering_op();

    // Released at the cap: dropped instead of lingering, so neighbours get their window
    CHECK(f.reader.count_downs() == 1);
    CHECK(!f.manager.is_field_up());
}

TEST_CASE("FieldManager power-cycles the field on demand", "[nfcv][field_manager]") {
    Fixture f;

    SECTION("fresh drops the field before arbitrating, even over an already dead one") {
        auto guard = f.manager.acquire(1, discovery_window_ms, Freshness::fresh);
        REQUIRE(guard);
        CHECK(f.reader.events == std::vector<FakeReader::Event> {
                  FakeReader::Down {},
                  FakeReader::Up { .antenna = 1 },
              });
    }

    SECTION("power_cycle drops and raises the field without releasing it") {
        auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
        REQUIRE(guard);
        f.reader.events.clear();

        REQUIRE(guard.power_cycle());
        CHECK(f.reader.events == std::vector<FakeReader::Event> {
                  FakeReader::Down {},
                  FakeReader::Up { .antenna = 0 },
              });
        CHECK(f.manager.is_field_up());
    }

    SECTION("power_cycle restamps the hold") {
        {
            auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
            REQUIRE(guard);

            fake.now_ms = hold_max_ms;
            REQUIRE(guard.power_cycle());
            guard.keep_field_up();
            f.reader.events.clear();
        }

        // The hold restarted at the cycle, so this release lingers rather than hitting the cap
        CHECK(f.reader.count_downs() == 0);
        CHECK(f.manager.is_field_up());
    }

    SECTION("a failed power_cycle leaves nothing radiating") {
        {
            // Acquired without retries, so the cycle gives up on the first collision too
            auto guard = f.manager.acquire(0, single_attempt, Freshness::reuse);
            REQUIRE(guard);

            f.reader.up_errors = { nfcv::Error::field_collision };
            CHECK(guard.power_cycle() == std::unexpected(nfcv::Error::field_collision));
            CHECK(!f.manager.is_field_up());

            f.reader.events.clear();
            guard.keep_field_up();
        }

        // Nothing to release - and no phantom linger armed over a dead field
        CHECK(f.reader.events.empty());
        CHECK(!f.manager.is_field_up());
    }
}

TEST_CASE("FieldManager arbitrates a contended field", "[nfcv][field_manager]") {
    Fixture f;

    SECTION("a free field costs a single attempt") {
        auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
        REQUIRE(guard);
        CHECK(f.reader.up_attempts == 1);
        CHECK(fake.delays.empty());
    }

    SECTION("persistent contention gives up at the deadline") {
        f.reader.always_fail_with = nfcv::Error::field_collision;

        auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
        CHECK(!guard);
        CHECK(guard.error() == nfcv::Error::field_collision);
        CHECK(f.reader.up_attempts > 1);
        CHECK(fake.now_ms >= io_window_ms);
    }

    SECTION("the caller's window is what decides how long we wait") {
        f.reader.always_fail_with = nfcv::Error::field_collision;

        REQUIRE(!f.manager.acquire(0, discovery_window_ms, Freshness::reuse));

        // A background scan gives up an order of magnitude sooner than an io operation would
        CHECK(fake.now_ms >= discovery_window_ms);
        CHECK(fake.now_ms < io_window_ms);
    }

    SECTION("a zero window is a single attempt") {
        f.reader.always_fail_with = nfcv::Error::field_collision;

        REQUIRE(!f.manager.acquire(0, single_attempt, Freshness::reuse));
        CHECK(f.reader.up_attempts == 1);
        CHECK(fake.delays.empty());
    }

    SECTION("every backoff stays inside the configured window") {
        f.reader.always_fail_with = nfcv::Error::field_collision;
        REQUIRE(!f.manager.acquire(0, io_window_ms, Freshness::reuse));

        REQUIRE(!fake.delays.empty());
        CHECK(std::ranges::all_of(fake.delays, [](uint32_t d) {
            return d >= nfcv::FieldManager::retry_delay_min_ms && d <= nfcv::FieldManager::retry_delay_max_ms;
        }));
    }

    SECTION("the whole backoff window is reachable") {
        // What seeding the platform RNG from the MCU unique ID buys on hardware: the ends of the
        // window are real, so two boards that lose the same round need not retry in lockstep
        f.reader.always_fail_with = nfcv::Error::field_collision;

        fake.random_values = { 0 };
        REQUIRE(!f.manager.acquire(0, io_window_ms, Freshness::reuse));
        CHECK(fake.delays.front() == nfcv::FieldManager::retry_delay_min_ms);

        Fixture other;
        other.reader.always_fail_with = nfcv::Error::field_collision;
        fake.random_values = { std::numeric_limits<uint32_t>::max() };
        REQUIRE(!other.manager.acquire(0, io_window_ms, Freshness::reuse));
        CHECK(fake.delays.front() == nfcv::FieldManager::retry_delay_max_ms);
    }

    SECTION("a mid-sequence success stops the retries") {
        f.reader.up_errors = { nfcv::Error::field_collision, nfcv::Error::field_collision };

        auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
        REQUIRE(guard);
        CHECK(f.reader.up_attempts == 3);
        CHECK(fake.delays.size() == 2);
        CHECK(f.manager.is_field_up());
    }

    SECTION("anything other than contention is reported straight away") {
        f.reader.up_errors = { nfcv::Error::timeout };

        auto guard = f.manager.acquire(0, io_window_ms, Freshness::reuse);
        CHECK(!guard);
        CHECK(guard.error() == nfcv::Error::timeout);
        CHECK(f.reader.up_attempts == 1);
    }

    SECTION("the deadline survives a clock wraparound") {
        Fixture late { std::numeric_limits<uint32_t>::max() - 100 };
        late.reader.always_fail_with = nfcv::Error::field_collision;

        CHECK(!late.manager.acquire(0, io_window_ms, Freshness::reuse));
        // Wrapped past zero rather than looping forever or bailing out immediately
        CHECK(fake.now_ms < io_window_ms);
        CHECK(late.reader.up_attempts > 1);
    }

    SECTION("the hold starts when arbitration finally wins, not when it started") {
        f.reader.up_errors = { nfcv::Error::field_collision, nfcv::Error::field_collision };
        f.lingering_op();

        // The backoff moved the clock; the cap must not count the time spent losing rounds
        const auto won_at = fake.now_ms;
        REQUIRE(won_at > 0);

        fake.now_ms = won_at + hold_max_ms - 1;
        f.lingering_op();
        CHECK(f.manager.is_field_up());
    }
}
