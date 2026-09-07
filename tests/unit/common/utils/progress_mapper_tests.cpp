#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

#include <test_utils/formatters.hpp>

#include <utils/progress_mapper.hpp>

enum class TestState {
    start = 0,
    next_1,
    next_2,
    next_3,
    not_used,
    not_used_2,
    finish
};

std::ostream &operator<<(std::ostream &out, const ProgressSpan &span) {
    out << "{" << int(span.min) << " - " << int(span.max) << "}";
    return out;
}

constexpr ProgressMapperWorkflowArray test_pipeline {
    std::to_array<ProgressMapperWorkflowStep<TestState>>({
        { TestState::start, 2 }, // 0-20
        { TestState::next_1, 4 }, // 20-60
        { TestState::next_2, 1 }, // 60-70
        { TestState::next_3, 1 }, // 70-80
        { TestState::finish, 2 }, // 80-100
    })
};

enum class ZeroScaleState {
    begin,
    middle,
    end,
};

constexpr ProgressMapperWorkflowArray pipeline_zero_scale_middle {
    std::to_array<ProgressMapperWorkflowStep<ZeroScaleState>>({
        { ZeroScaleState::begin, 1 },
        { ZeroScaleState::middle, 0 },
        { ZeroScaleState::end, 1 },
    })
};

constexpr ProgressMapperWorkflowArray pipeline_zero_scale_end {
    std::to_array<ProgressMapperWorkflowStep<ZeroScaleState>>({
        { ZeroScaleState::begin, 1 },
        { ZeroScaleState::middle, 1 },
        { ZeroScaleState::end, 0 },
    })
};

TEST_CASE("ProgressMapper: Pipeline test") {
    CHECK(test_pipeline.steps().size() == size_t(5));
    CHECK(test_pipeline.scale_sum() == 2 + 4 + 1 + 1 + 2);
}

TEST_CASE("ProgressMapper: Natural Progress test") {
    ProgressMapper<TestState> mapper(test_pipeline);
    CHECK(mapper.update_progress_span(TestState::start) == ProgressSpan { 0, 20 });

    CHECK(mapper.update_progress_span(TestState::next_1) == ProgressSpan { 20, 60 });
    CHECK(mapper.update_progress(TestState::next_1, 0.5) == 40);
    CHECK(mapper.update_progress(TestState::next_1, 1) == 60);

    CHECK(mapper.update_progress_span(TestState::next_2) == ProgressSpan { 60, 70 });

    CHECK(mapper.update_progress_span(TestState::next_3) == ProgressSpan { 70, 80 });
    CHECK(mapper.update_progress(TestState::next_3, 1) == 80);

    CHECK(mapper.update_progress_span(TestState::finish) == ProgressSpan { 80, 100 });
}

TEST_CASE("ProgressMapper: Skipped Progress test") {
    ProgressMapper<TestState> mapper(test_pipeline);
    // Skipped first two states
    CHECK(mapper.update_progress_span(TestState::next_2) == ProgressSpan { 0, 25 });

    // Skipped second to last state.
    CHECK(mapper.update_progress_span(TestState::finish) == ProgressSpan { 25, 100 });

    // This method does not alter current progress
    CHECK(mapper.current_progress() == 0.0f);
}

TEST_CASE("ProgressMapper: Skip around") {
    ProgressMapper<TestState> mapper(test_pipeline);

    // Skip start(2), go directly to next_1
    // start's weight (2) distributed among remaining 4 states with the cumualtive sum of 8
    // next_1 has scale 4, so it should get a progress span of 0-50
    CHECK(mapper.update_progress_span(TestState::next_1) == ProgressSpan { 0, 50 });

    CHECK(mapper.update_progress(TestState::next_1, 0.5f) == 25);

    // Skipping state 2

    CHECK(mapper.update_progress_span(TestState::next_3) == ProgressSpan { 50, 67 });
    CHECK(mapper.update_progress(TestState::next_3, 0.1f) == 51);
    CHECK(mapper.update_progress(TestState::next_3, 1.0f) == 67);
    CHECK(mapper.update_progress(TestState::finish, 0.0f) == 67);

    // We are regressing, so this should redistribute the workflow, next_1 should get default 20-60
    CHECK(mapper.update_progress_span(TestState::next_1) == ProgressSpan { 20, 60 });
    CHECK(mapper.update_progress(TestState::next_1, 0.0f) == 20);

    // Skip back to finish, since we should be operating in range 60 - 100
    CHECK(mapper.update_progress(TestState::finish, 0.5f) == 80);
    CHECK(mapper.update_progress(TestState::finish, 0.75f) == 90);
    CHECK(mapper.update_progress(TestState::finish, 1.0f) == 100);
}

TEST_CASE("ProgressMapper: Improvised states") {
    static constexpr ProgressMapperWorkflowArray workflow {
        std::to_array<ProgressMapperWorkflowStep<TestState>>({
            { TestState::start, 1 },
            { TestState::next_1, 1 },
            { TestState::next_2, 1 },
            { TestState::finish, 1 },
        })
    };

    ProgressMapper<TestState> mapper(workflow);

    // 4 remaining scale + 1 improvised
    CHECK(mapper.update_progress_span(TestState::not_used) == ProgressSpan { 0, 20 });

    CHECK(mapper.update_progress_span(TestState::start) == ProgressSpan { 20, 40 });
    CHECK(mapper.update_progress_span(TestState::next_1) == ProgressSpan { 40, 60 });

    // 60-100, 2 remaining scale + 1 improvised
    CHECK(mapper.update_progress_span(TestState::not_used) == ProgressSpan { 60, 73 });
    CHECK(mapper.update_progress_span(TestState::next_2) == ProgressSpan { 73, 87 });

    // Regression resets the state
    CHECK(mapper.update_progress_span(TestState::next_1) == ProgressSpan { 25, 50 });

    // Subsequent alterations slice more and more from the pie, every item smaller
    CHECK(mapper.update_progress_span(TestState::not_used) == ProgressSpan { 50, 67 });
    CHECK(mapper.update_progress_span(TestState::not_used_2) == ProgressSpan { 67, 78 });
    CHECK(mapper.update_progress_span(TestState::not_used) == ProgressSpan { 78, 85 });

    // Finish skips next_1 and next_2 and finishes
    CHECK(mapper.update_progress_span(TestState::finish) == ProgressSpan { 85, 100 });

    // Rogue states after that don't have any more space
    CHECK(mapper.update_progress_span(TestState::not_used) == ProgressSpan { 100, 100 });
}

TEST_CASE("ProgressMapper: Update current progress") {
    ProgressMapper<TestState> mapper(test_pipeline);
    CHECK(mapper.current_progress() == 0);
    CHECK(mapper.update_progress(TestState::start, to_normalized_progress(0, 40, 10)) == 5);
    CHECK(mapper.current_progress() == 5);
    CHECK(mapper.update_progress(TestState::start, to_normalized_progress(0, 40, 20)) == 10);
    CHECK(mapper.current_progress() == 10);
    CHECK(mapper.update_progress(TestState::start, to_normalized_progress(0, 40, 30)) == 15);
    CHECK(mapper.current_progress() == 15);
}

TEST_CASE("ProgressMapper: Zero-scale step (visited)") {
    ProgressMapper<ZeroScaleState> mapper(pipeline_zero_scale_middle);
    CHECK(mapper.update_progress_span(ZeroScaleState::begin) == ProgressSpan { 0, 50 });
    CHECK(mapper.update_progress(ZeroScaleState::middle, 0.5f) == 50);
    CHECK(mapper.update_progress(ZeroScaleState::middle, 1.0f) == 50);
    CHECK(mapper.update_progress_span(ZeroScaleState::end) == ProgressSpan { 50, 100 });
    CHECK(mapper.update_progress(ZeroScaleState::end, 1.0f) == 100);
}

TEST_CASE("ProgressMapper: Zero-scale step (skipped)") {
    ProgressMapper<ZeroScaleState> mapper(pipeline_zero_scale_middle);
    CHECK(mapper.update_progress(ZeroScaleState::begin, 1) == 50);
    CHECK(mapper.update_progress_span(ZeroScaleState::end) == ProgressSpan { 50, 100 });
    CHECK(mapper.update_progress(ZeroScaleState::end, 1.0f) == 100);
}

TEST_CASE("ProgressMapper: Trailing zero-scale step does not divide by zero") {
    ProgressMapper<ZeroScaleState> mapper(pipeline_zero_scale_end);
    CHECK(mapper.update_progress_span(ZeroScaleState::begin) == ProgressSpan { 0, 50 });
    CHECK(mapper.update_progress(ZeroScaleState::middle, 1.0f) == 100);
    CHECK(mapper.update_progress_span(ZeroScaleState::end) == ProgressSpan { 100, 100 });
}
