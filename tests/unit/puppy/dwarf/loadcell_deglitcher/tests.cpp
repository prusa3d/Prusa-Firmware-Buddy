#include <catch2/catch_test_macros.hpp>
#include <deque>
#include <limits>
#include <optional>

#include <loadcell_deglitcher.hpp>

using dwarf::loadcell::LoadcellDeglitcher;
using fifo_coder::LoadcellRecord;

// Convenience: build a LoadcellRecord with a signed raw value and sequential timestamp.
static LoadcellRecord make_record(uint32_t timestamp, int32_t raw) {
    return LoadcellRecord { timestamp, static_cast<uint32_t>(raw) };
}

// Drain all available output samples from the deglitcher into a vector.
static std::vector<LoadcellRecord> drain(LoadcellDeglitcher &d, std::deque<LoadcellRecord> &input) {
    std::vector<LoadcellRecord> out;
    auto try_get = [&input](LoadcellRecord &s) -> bool {
        if (input.empty()) {
            return false;
        }
        s = input.front();
        input.pop_front();
        return true;
    };
    for (std::optional<LoadcellRecord> rec = d.next(try_get); rec.has_value(); rec = d.next(try_get)) {
        out.push_back(*rec);
    }
    return out;
}

// Base value well within normal operating range.
static constexpr int32_t BASE = 1000000;
// A jump large enough to stay out-of-threshold for several samples despite the
// running average creeping toward it (so MAX_SKIPPED suspects can accumulate).
static constexpr int32_t BIG_JUMP = 2 * LoadcellDeglitcher::MAX_DIFFERENCE;

TEST_CASE("All in-threshold samples pass through in order") {
    LoadcellDeglitcher d;
    std::deque<LoadcellRecord> input;

    // Prime the average a bit so small in-threshold values don't look odd.
    for (uint32_t i = 0; i < 5; ++i) {
        input.push_back(make_record(i, BASE));
    }
    std::vector<LoadcellRecord> result = drain(d, input);

    REQUIRE(result.size() == 5);
    for (uint32_t i = 0; i < 5; ++i) {
        CHECK(result[i].timestamp == i);
        CHECK(static_cast<int32_t>(result[i].loadcell_raw_value) == BASE);
    }
}

TEST_CASE("Short glitch (1–3 suspects) that reverts is dropped") {
    LoadcellDeglitcher d;
    std::deque<LoadcellRecord> input;

    // Prime average with several good samples.
    for (uint32_t i = 0; i < 10; ++i) {
        input.push_back(make_record(i, BASE));
    }
    // 2 out-of-threshold samples (glitch), then back to base.
    input.push_back(make_record(10, BASE + BIG_JUMP));
    input.push_back(make_record(11, BASE + BIG_JUMP));
    input.push_back(make_record(12, BASE)); // reverts → glitch dropped
    input.push_back(make_record(13, BASE));

    std::vector<LoadcellRecord> result = drain(d, input);

    // The glitch (10, 11) is dropped; everything else passes through in order.
    std::vector<uint32_t> expected = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13 };
    REQUIRE(result.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        CHECK(result[i].timestamp == expected[i]);
    }
}

TEST_CASE("Sustained change (> MAX_SKIPPED suspects) flushed with original values") {
    LoadcellDeglitcher d;
    std::deque<LoadcellRecord> input;

    // Prime average.
    for (uint32_t i = 0; i < 10; ++i) {
        input.push_back(make_record(i, BASE));
    }
    // A full suspect burst (MAX_SKIPPED + 1) is confirmed as signal and flushed.
    const size_t sustained = LoadcellDeglitcher::MAX_SKIPPED + 1;
    for (uint32_t i = 0; i < sustained; ++i) {
        input.push_back(make_record(10 + i, BASE + BIG_JUMP));
    }

    std::vector<LoadcellRecord> result = drain(d, input);

    // Nothing is dropped: 10 primed samples + the whole flushed burst, in order.
    REQUIRE(result.size() == 10 + sustained);
    for (uint32_t i = 0; i < sustained; ++i) {
        const LoadcellRecord &r = result[10 + i];
        CHECK(r.timestamp == 10 + i);
        CHECK(static_cast<int32_t>(r.loadcell_raw_value) == BASE + BIG_JUMP);
    }
}

TEST_CASE("Boundary: exactly MAX_SKIPPED suspects reverts → dropped; MAX_SKIPPED+1 → flushed") {
    SECTION("MAX_SKIPPED suspects then revert → all dropped") {
        LoadcellDeglitcher d;
        std::deque<LoadcellRecord> input;

        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        for (size_t i = 0; i < LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            input.push_back(make_record(10 + static_cast<uint32_t>(i), BASE + BIG_JUMP));
        }
        // Revert — this triggers the drop.
        input.push_back(make_record(10 + LoadcellDeglitcher::MAX_SKIPPED, BASE));

        std::vector<LoadcellRecord> result = drain(d, input);

        for (const LoadcellRecord &r : result) {
            for (size_t i = 0; i < LoadcellDeglitcher::MAX_SKIPPED; ++i) {
                CHECK(r.timestamp != 10 + static_cast<uint32_t>(i));
            }
        }
    }

    SECTION("MAX_SKIPPED+1 suspects → all flushed") {
        LoadcellDeglitcher d;
        std::deque<LoadcellRecord> input;

        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        for (size_t i = 0; i <= LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            input.push_back(make_record(10 + static_cast<uint32_t>(i), BASE + BIG_JUMP));
        }

        std::vector<LoadcellRecord> result = drain(d, input);

        for (size_t i = 0; i <= LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            bool found = false;
            for (const LoadcellRecord &r : result) {
                if (r.timestamp == 10 + static_cast<uint32_t>(i)) {
                    found = true;
                }
            }
            CHECK(found);
        }
    }
}

TEST_CASE("Deferred decision across input exhaustion: no samples lost") {
    LoadcellDeglitcher d;

    // Prime with good samples.
    {
        std::deque<LoadcellRecord> input;
        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        drain(d, input);
    }

    // Feed MAX_SKIPPED suspect samples, then exhaust the input.
    size_t emitted_first_half = 0;
    {
        std::deque<LoadcellRecord> input;
        for (size_t i = 0; i < LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            input.push_back(make_record(10 + static_cast<uint32_t>(i), BASE + BIG_JUMP));
        }
        auto try_get = [&input](LoadcellRecord &s) -> bool {
            if (input.empty()) {
                return false;
            }
            s = input.front();
            input.pop_front();
            return true;
        };
        while (d.next(try_get).has_value()) {
            emitted_first_half++;
        }
        // Suspects are held; try_get exhausted; nothing emitted yet (no decision).
        CHECK(emitted_first_half == 0);
    }

    // Resume with one more suspect (crosses MAX_SKIPPED) → flush all.
    {
        std::deque<LoadcellRecord> input;
        input.push_back(make_record(10 + LoadcellDeglitcher::MAX_SKIPPED, BASE + BIG_JUMP));
        std::vector<LoadcellRecord> result = drain(d, input);

        // All MAX_SKIPPED+1 suspects must be emitted.
        CHECK(result.size() == LoadcellDeglitcher::MAX_SKIPPED + 1);
        for (size_t i = 0; i <= LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            bool found = false;
            for (const LoadcellRecord &r : result) {
                if (r.timestamp == 10 + static_cast<uint32_t>(i)) {
                    found = true;
                }
            }
            CHECK(found);
        }
    }
}

TEST_CASE("discard_pending drops held suspects and resets flushing state") {
    LoadcellDeglitcher d;

    // Prime average.
    {
        std::deque<LoadcellRecord> input;
        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        drain(d, input);
    }

    // Feed some suspects to hold state.
    {
        std::deque<LoadcellRecord> input;
        for (size_t i = 0; i < LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            input.push_back(make_record(10 + static_cast<uint32_t>(i), BASE + BIG_JUMP));
        }
        drain(d, input);
    }

    // Discard pending — held suspects should be gone.
    d.discard_pending();

    // After discard, feeding a good sample should produce output immediately (no flush).
    std::deque<LoadcellRecord> input;
    input.push_back(make_record(20, BASE));
    std::vector<LoadcellRecord> result = drain(d, input);

    // Only the one good sample, none of the suspects.
    REQUIRE(result.size() == 1);
    CHECK(result[0].timestamp == 20);
    for (size_t i = 0; i < LoadcellDeglitcher::MAX_SKIPPED; ++i) {
        CHECK(result[0].timestamp != 10 + static_cast<uint32_t>(i));
    }
}

TEST_CASE("Undefined sentinel (INT32_MIN) does not corrupt average and is treated as out-of-threshold") {
    static constexpr int32_t UNDEF = LoadcellDeglitcher::undefined_value;

    SECTION("Single undefined sample is treated as a glitch and dropped if it reverts") {
        LoadcellDeglitcher d;
        std::deque<LoadcellRecord> input;

        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        input.push_back(make_record(10, UNDEF));
        input.push_back(make_record(11, BASE)); // reverts

        std::vector<LoadcellRecord> result = drain(d, input);

        // The undefined sentinel must not appear.
        for (const LoadcellRecord &r : result) {
            CHECK(r.timestamp != 10);
        }
        // Good sample after must appear.
        bool found_11 = false;
        for (const LoadcellRecord &r : result) {
            if (r.timestamp == 11) {
                found_11 = true;
            }
        }
        CHECK(found_11);
    }

    SECTION("Sustained undefined run (> MAX_SKIPPED) is flushed with original values") {
        LoadcellDeglitcher d;
        std::deque<LoadcellRecord> input;

        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        const size_t sustained = LoadcellDeglitcher::MAX_SKIPPED + 1;
        for (uint32_t i = 0; i < sustained; ++i) {
            input.push_back(make_record(10 + i, UNDEF));
        }

        std::vector<LoadcellRecord> result = drain(d, input);

        // All undefined samples must appear in output.
        for (uint32_t i = 0; i < sustained; ++i) {
            bool found = false;
            for (const LoadcellRecord &r : result) {
                if (r.timestamp == 10 + i) {
                    found = true;
                    CHECK(static_cast<int32_t>(r.loadcell_raw_value) == UNDEF);
                }
            }
            CHECK(found);
        }
    }

    SECTION("Average is not corrupted by undefined: good samples still pass after") {
        LoadcellDeglitcher d;
        std::deque<LoadcellRecord> input;

        // Prime.
        for (uint32_t i = 0; i < 10; ++i) {
            input.push_back(make_record(i, BASE));
        }
        // Sustained undefined run that gets flushed.
        for (uint32_t i = 0; i <= LoadcellDeglitcher::MAX_SKIPPED; ++i) {
            input.push_back(make_record(10 + i, UNDEF));
        }
        // Good sample close to original BASE — should pass if average was not corrupted.
        input.push_back(make_record(10 + LoadcellDeglitcher::MAX_SKIPPED + 1, BASE));

        std::vector<LoadcellRecord> result = drain(d, input);

        // The final good sample must appear (average not corrupted by INT32_MIN).
        bool found_good = false;
        for (const LoadcellRecord &r : result) {
            if (r.timestamp == 10 + LoadcellDeglitcher::MAX_SKIPPED + 1) {
                found_good = true;
            }
        }
        CHECK(found_good);
    }
}
