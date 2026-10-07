/// @file
#pragma once

#include <cmath>
#include <variant>
#include <expected>

#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <async_job/async_job.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <utils/timing/rate_limiter.hpp>
#include <inplace_function.hpp>
#include <freertos/mutex.hpp>

namespace buddy::openprinttag {

/// Class that handles writing filament usage to OpenPrintTag
/// Public functions are thread-safe
class FilamentUsageTracker {
    friend FilamentUsageTracker &filament_usage_tracker();
    friend class FilamentUsageTrackerTester;

public:
    /// Hof often step() runs
    /// The system doesn't have to be that responsive, so we can save some performance
    static constexpr uint32_t step_limiter_interval_ms = 51;

    /// How often we automatically flush consumed filament
    /// SLIX2 have write endurance of 100.000 cycles.
    /// We aim for 1 year of constant writes, so 365 * 24 * 60 / 100000 = ~one write every 5 minutes
    static constexpr uint32_t flush_timer_interval_ms = 5 * 60 * 1000;

    /// Returned by some functions if the filament tracking is impossible (the tag doesn't support it or no tag assigned)
    struct TrackingImpossible {};

public:
    /// Do one step of whatever the tracker is doing
    void step();

    struct FlushArgs {
        std::variant<VirtualToolIndex, AllTools> tools;

        /// If set, shows a warning if the write fails
        bool warn_on_failure = false;
    };

    /// Marks the specified tool(s) for immediate write of the usage
    /// The operation is still not blocking, one needs to check @p is_write_pending
    void flush(const FlushArgs &args);

    /// @returns extruded distance (in mm) that is not yet committed to the tag
    [[nodiscard]] uint32_t uncommited_consumption_mm(VirtualToolIndex tool) const;

    struct FilamentReport {
        /// Remaining material
        float remaining_weight_g;

        /// Full package weight (just the material)
        float full_weight_g;

        /// How many grams a millimeter of filament weighs
        float g_per_mm;

        /// Whether the full weight is nominal or actual
        bool full_weight_is_nominal : 1;
    };
    [[nodiscard]] std::optional<FilamentReport> filament_report(VirtualToolIndex tool) const;

    /// @returns whether usage tracking is active for the specified tool
    bool is_tracking(VirtualToolIndex tool) const;

private:
    FilamentUsageTracker() = default;

    /// Increase @p current_tool_ to the next tool
    void next_tool();

private:
    struct ToolData {
        /// extruded_length (in mm) * g_per_mm = consumed_weight (in g)
        /// Filled in during init (see @p init_pending )
        float g_per_mm = NAN;

        /// Full weight of the spool (in g)
        float full_weight_g = NAN;

        /// FilamentTracker::get_extruded_distance that corresponds to the current consumed_weight value in the tag
        /// Pending distance is current FilamentTracker::get_extruded_distance - base_extruded_distance
        uint32_t base_extruded_distance_mm;

        /// Remaining filament on the spool that has been already commited
        float base_remaining_filament_g = NAN;

        /// Tag we're currently tracking
        ToolTag::UIDHash assigned_tag = ToolTag::no_tag_hash;

        /// Whether the tag has a pending initialization procedure
        /// The procedure fills @p g_per_mm
        bool init_pending : 1 = false;

        /// Whether we're supposed to update the consumed weight
        bool write_pending : 1 = false;

        /// If the next job fails, show a warning
        /// Used to warn the user about failing openprinttag for example at the end of the print
        /// Resets after the job is complete
        bool warn_if_next_write_fails : 1 = false;

        /// If something happens that prevents tracking and is not recoverable by retrying, gets set to true
        /// Set to true by defalut because there is not tag assigned
        bool unrecoverable_error : 1 = true;

        /// If full_weight is present, it can be either nominal or actual.
        bool full_weight_is_nominal : 1 = false;
    };
    // Make sure the struct is not too big, it will be instantiated for each virtual tool (>=10 for INDX)
    static_assert(sizeof(ToolData) == 20);

    StrongIndexArray<ToolData, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> tool_data_;

    struct AsyncJobFinishCallbackArgs {
        FilamentUsageTracker &tracker;
        ToolData &tool_data;
        VirtualToolIndex tool;
    };

    using AsyncJobFinishCallback = stdext::inplace_function<void(const AsyncJobFinishCallbackArgs &args), 16>;

    /// Single job slot for doing the OPT IO asynchronously
    /// The result is a function that should be called once the async job finishes
    /// The running async_job is always related to @p current_tool_
    AsyncJobWithResult<AsyncJobFinishCallback> async_job_;

    /// Tool we're currently handling within @p step() or @p async_job_
    VirtualToolIndex current_tool_ = VirtualToolIndex::from_raw(0);

    /// This guy calls @p flush(AllTools{}) every so often so that
    RateLimiter<uint32_t> flush_timer_ms_ { flush_timer_interval_ms };

    /// The step() function processing doesn't need to be that often
    RateLimiter<uint32_t> step_limiter_ms_ { step_limiter_interval_ms };

private:
    void flush_nolock(const FlushArgs &args);

    [[nodiscard]] uint32_t uncommited_consumption_mm_nolock(VirtualToolIndex tool) const;

    /// Finish callback when we determine that the tag cannot be used for filament tracking, without change of recovery
    static void cannot_track_finish_cb(const AsyncJobFinishCallbackArgs &args);

    /// Finish callback when something failed, but retrying might help
    static void retry_finish_cb_impl(ToolData &tool_data);

    static void retry_finish_cb(const AsyncJobFinishCallbackArgs &args) {
        retry_finish_cb_impl(args.tool_data);
    }

    static AsyncJobFinishCallback tool_init_async(AsyncJobExecutionControl &ctrl, ToolTag tag);

    struct WriteConsumptionArgs {
        ToolTag tag;
        uint32_t extruded_distance_delta_mm;
        float g_per_mm;
    };

    static AsyncJobFinishCallback write_consumption_async(AsyncJobExecutionControl &ctrl, const WriteConsumptionArgs &args);

    /// write_consumption_async arguments do not fit to a stdext::inplace_function storage for the async job, so we have to store them separately
    std::optional<WriteConsumptionArgs> write_consumption_args_;

    mutable freertos::Mutex mutex_;
};

/// @returns instance of the FilamentUsageTracker singleton
FilamentUsageTracker &filament_usage_tracker();

} // namespace buddy::openprinttag
