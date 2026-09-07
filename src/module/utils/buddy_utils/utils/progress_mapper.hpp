#pragma once

#include <span>
#include <array>
#include <limits>
#include <cmath>
#include <optional>

#include <utils/uncopyable.hpp>
#include <utils/progress.hpp>
#include <bsod/bsod.h>

template <class State>
struct ProgressMapperWorkflowStep {
    using Scale = uint8_t;

    /// External key (presumably FSM state) associated with the step
    State state;

    /// 'Size' of the step, relative to other steps.
    /// Determines how much progress span is associated with it
    Scale scale = 1;
};

template <class State>
class ProgressMapperWorkflow : Uncopyable {

public:
    using StepIndex = uint8_t;
    using Step = ProgressMapperWorkflowStep<State>;
    using StepScale = Step::Scale;

    struct Runtime {};

    struct StepData {
        /// Same as ProgressMapperWorkflowStep::state
        State state;

        /// Similar to ProgressMapperWorkflowStep::scale, but with scale of all previous steps accumulated
        uint8_t cumulative_scale;
    };

public:
    consteval ProgressMapperWorkflow() = default;
    constexpr ProgressMapperWorkflow(Runtime) {}

    constexpr const auto &steps() const {
        return steps_;
    }

    constexpr inline StepScale scale_sum() const {
        return steps_.back().cumulative_scale;
    }

    constexpr inline StepScale avg_step_scale() const {
        return std::max<StepScale>(scale_sum() / steps_.size(), 1);
    }

    constexpr static inline bool is_workflow_valid(StepIndex step_count) {
        return (step_count > 0) && (step_count < std::numeric_limits<StepIndex>::max());
    }

protected:
    consteval void setup(const std::span<StepData> &steps, const std::span<const Step> &params) {
        setup(Runtime {}, steps, params);
    }

    constexpr void setup(Runtime, const std::span<StepData> &steps, const std::span<const Step> &params) {
        debug_assert(is_workflow_valid(params.size()));
        debug_assert(params.size() == steps.size());

        steps_ = steps;
        StepScale scale_accum = 0;

        auto step = steps.begin();
        for (const auto &param : params) {
            scale_accum += param.scale;

            *step = StepData {
                .state = param.state,
                .cumulative_scale = scale_accum,
            };
            step++;
        }

        debug_assert(scale_accum > 0);
    }

private:
    std::span<const StepData> steps_;
};

template <class State, std::size_t N>
class ProgressMapperWorkflowArray : public ProgressMapperWorkflow<State> {
public:
    using WorkflowBase = ProgressMapperWorkflow<State>;
    using Runtime = typename WorkflowBase::Runtime;

public:
    consteval ProgressMapperWorkflowArray(const std::array<ProgressMapperWorkflowStep<State>, N> &params) {
        // max is reserved as for the initial value
        static_assert(WorkflowBase::is_workflow_valid(N));

        this->setup(data_, params);
    }

    constexpr ProgressMapperWorkflowArray(Runtime, const std::array<ProgressMapperWorkflowStep<State>, N> &params)
        : WorkflowBase(Runtime {}) {
        // max is reserved as for the initial value
        static_assert(WorkflowBase::is_workflow_valid(N));

        this->setup(Runtime {}, data_, params);
    }

private:
    std::array<typename WorkflowBase::StepData, N> data_;
};

class BaseProgressMapper : Uncopyable {

public:
    using StepIndex = uint8_t;
    static constexpr StepIndex invalid_step = std::numeric_limits<StepIndex>::max();

public:
    inline ProgressPercent current_progress() const {
        return current_progress_;
    }

protected:
    BaseProgressMapper() = default;

protected:
    ProgressPercent current_progress_ = 0;

    /// Start progress of the origin step
    ProgressSpan current_step_span_;

    /// Index of the current step of the mapper
    /// Does not get updated on states that are outside of the workflow
    StepIndex last_valid_step_index_ = invalid_step;
};

template <class State>
class ProgressMapper : public BaseProgressMapper {

public:
    using Workflow = ProgressMapperWorkflow<State>;
    using StepIndex = Workflow::StepIndex;
    using StepScale = Workflow::StepScale;

public:
    ProgressMapper() = default;
    ProgressMapper(const Workflow &workflow) {
        setup(workflow);
    }

    /// Reset the progress mapper and set it up for the specified workflow
    void setup(const Workflow &workflow) {
        workflow_ = &workflow;
        current_progress_ = 0;
        current_step_span_ = ProgressSpan { 0, 0 };
        last_valid_step_index_ = invalid_step;
        current_state_ = std::nullopt;
    }

    /// Informs the progress mapper that the process in currently in \p state.
    /// \param normalized_progress determines progress within the current step
    /// \returns overall progress within the workflow
    ProgressPercent update_progress(const State state, const float normalized_progress) {
        current_progress_ = update_progress_span(state).map(normalized_progress);
        return current_progress_;
    }

    /// Informs the progress mapper that the process in currently in \p state.
    /// \returns progress span of the current state
    ProgressSpan update_progress_span(const State state) {
        if (!workflow_) {
            return ProgressSpan { 0, 0 };
        }

        if (state == current_state_) {
            // Same state, same span
            return current_step_span_;
        }

        // Default step scale for items out of workflow
        StepScale step_scale = workflow_->avg_step_scale();

        const auto step_it = std::ranges::find_if(workflow_->steps(), [state](const auto &step) { return step.state == state; });
        if (step_it != workflow_->steps().end()) {
            const auto step_index = step_it - workflow_->steps().begin();
            const auto previous_step_cumulative_scale = ((step_index == 0) ? 0 : std::prev(step_it)->cumulative_scale);

            if (step_index < last_valid_step_index_ && last_valid_step_index_ != invalid_step) {
                // If we've regressed in the workflow, "reset" the smart scaling and assume default workflow scales
                // This updates current_step_span_.min below
                current_step_span_.max = static_cast<ProgressPercent>(std::roundf(float(previous_step_cumulative_scale) / workflow_->scale_sum() * 100.0f));
            }

            last_valid_step_index_ = step_index;
            step_scale = step_it->cumulative_scale - previous_step_cumulative_scale;
        }

        // This step starts where the previous step ended
        current_step_span_.min = current_step_span_.max;

        const StepScale remaining_scale = //
            workflow_->scale_sum() - ((last_valid_step_index_ == invalid_step) ? 0 : workflow_->steps()[last_valid_step_index_].cumulative_scale)
            + step_scale;

        if (remaining_scale > 0) {
            current_step_span_.max += static_cast<int>(std::roundf(float(step_scale) / remaining_scale * (100 - current_step_span_.min)));
        }

        current_state_ = state;
        return current_step_span_;
    }

private:
    const Workflow *workflow_ = nullptr;
    std::optional<State> current_state_ = std::nullopt;
};
