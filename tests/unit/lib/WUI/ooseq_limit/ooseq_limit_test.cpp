#include <ooseq_limit.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace ooseq_limit;

TEST_CASE("ooseq_limit empty state", "[ooseq_limit]") {
    CHECK(compute_bytes_limit(true, 0, 0) == INBOUND_PCB_BYTES);
    CHECK(compute_bytes_limit(false, 0, 0) == OUTBOUND_PCB_BYTES);
    CHECK(compute_segs_limit(true, 0, 0) == INBOUND_PCB_SEGS);
    CHECK(compute_segs_limit(false, 0, 0) == OUTBOUND_PCB_SEGS);
}

TEST_CASE("ooseq_limit inbound saturates at its share, not per-pcb cap", "[ooseq_limit]") {
    // Other inbound pcbs already hold the whole inbound share; per-pcb cap
    // alone would still allow more.
    CHECK(compute_bytes_limit(true, INBOUND_SHARE_BYTES, INBOUND_SHARE_BYTES) == 0);
    CHECK(compute_segs_limit(true, INBOUND_SHARE_SEGS, INBOUND_SHARE_SEGS) == 0);
}

TEST_CASE("ooseq_limit outbound holding the full global budget clamps a new inbound pcb to zero", "[ooseq_limit]") {
    CHECK(compute_bytes_limit(true, GLOBAL_BYTES, 0) == 0);
}

TEST_CASE("ooseq_limit tiny inbound segment flood cannot starve outbound below its guaranteed segments", "[ooseq_limit]") {
    // Inbound floods the whole inbound segment share with near-zero-byte
    // segments; outbound must still get its guaranteed GLOBAL_SEGS -
    // INBOUND_SHARE_SEGS segments.
    constexpr uint16_t other_all_segs = INBOUND_SHARE_SEGS;
    constexpr uint16_t other_inbound_segs = INBOUND_SHARE_SEGS;
    const uint16_t outbound_limit = compute_segs_limit(false, other_all_segs, other_inbound_segs);
    CHECK(outbound_limit == GLOBAL_SEGS - INBOUND_SHARE_SEGS);
}

TEST_CASE("ooseq_limit max(0, ...) clamps instead of underflowing", "[ooseq_limit]") {
    // Other pcbs already exceed the share/global budgets - must clamp to 0,
    // not wrap around through unsigned underflow.
    CHECK(compute_bytes_limit(true, GLOBAL_BYTES + 1, INBOUND_SHARE_BYTES + 1) == 0);
    CHECK(compute_bytes_limit(false, GLOBAL_BYTES + 1, 0) == 0);
    CHECK(compute_segs_limit(true, GLOBAL_SEGS + 1, INBOUND_SHARE_SEGS + 1) == 0);
    CHECK(compute_segs_limit(false, GLOBAL_SEGS + 1, 0) == 0);
}

TEST_CASE("ooseq_limit outbound ignores inbound-only usage", "[ooseq_limit]") {
    // Outbound isn't bound by the inbound share at all, only by its own
    // per-pcb cap and the global budget.
    CHECK(compute_bytes_limit(false, 0, INBOUND_SHARE_BYTES) == OUTBOUND_PCB_BYTES);
    CHECK(compute_segs_limit(false, 0, INBOUND_SHARE_SEGS) == OUTBOUND_PCB_SEGS);
}
