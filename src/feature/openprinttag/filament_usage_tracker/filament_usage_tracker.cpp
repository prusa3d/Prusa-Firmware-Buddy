#include "filament_usage_tracker.hpp"

#include <mutex>

#include <feature/filament_tracker/filament_tracker.hpp>
#include <feature/openprinttag/requests_read_multi.hpp>
#include <feature/openprinttag/requests_write.hpp>
#include <feature/openprinttag/data_utils.hpp>
#include <freertos/timing.hpp>
#include <logging/log.hpp>
#include <marlin_server.hpp>
#include <raii/scope_guard.hpp>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(OpenPrintTag);

namespace buddy::openprinttag {

FilamentUsageTracker &filament_usage_tracker() {
    static FilamentUsageTracker instance;
    return instance;
}

void FilamentUsageTracker::flush(const FlushArgs &args) {
    std::lock_guard _lg(mutex_);
    return flush_nolock(args);
}

void FilamentUsageTracker::flush_nolock(const FlushArgs &args) {
    for (VirtualToolIndex tool : tool_index_iterator(args.tools)) {
        // We need this check to prevent setting warn_if_next_write_fails
        // and then popping up warnings if init fails
        if (uncommited_consumption_mm_nolock(tool) == 0) {
            continue;
        }

        ToolData &td = tool_data_[tool];
        td.write_pending = true;
        td.warn_if_next_write_fails |= args.warn_on_failure;
    }
}

uint32_t FilamentUsageTracker::uncommited_consumption_mm(VirtualToolIndex tool) const {
    std::lock_guard _lg(mutex_);
    return uncommited_consumption_mm_nolock(tool);
}

uint32_t FilamentUsageTracker::uncommited_consumption_mm_nolock(VirtualToolIndex tool) const {
    const uint32_t extruded_distance = filament_tracker().get_extruded_distance(tool);
    return extruded_distance - std::min<uint32_t>(extruded_distance, tool_data_[tool].base_extruded_distance_mm);
}

std::optional<float> FilamentUsageTracker::remaining_filament_g(VirtualToolIndex tool) const {
    std::lock_guard _lg(mutex_);

    const auto &tool_data = tool_data_[tool];
    if (std::isnan(tool_data.base_remaining_filament_g)) {
        return std::nullopt;
    }

    return std::max<float>(0, tool_data.base_remaining_filament_g - uncommited_consumption_mm_nolock(tool) * tool_data.g_per_mm);
}

bool FilamentUsageTracker::is_tracking(VirtualToolIndex tool) const {
    std::lock_guard _lg(mutex_);

    const auto &tool_data = tool_data_[tool];
    return !tool_data.init_pending && !tool_data.unrecoverable_error;
}

void FilamentUsageTracker::step() {
#ifndef UNITTESTS
    debug_assert(marlin_server::is_marlin_server_thread());
#endif

    std::lock_guard _lg(mutex_);

    if (!step_limiter_ms_.check(freertos::millis())) {
        return;
    }

    if (flush_timer_ms_.check(freertos::millis())) {
        flush_nolock({
            .tools = AllTools {},
            .warn_on_failure = false,
        });
    }

    if (async_job_.is_active()) {
        // Always let the async job finish its job first
        return;
    }

    // Cycle tools each step, unless we've issued an async job
    const ScopeGuard next_tool_guard = [&] {
        if (async_job_.state() == AsyncJobBase::State::idle) {
            current_tool_ = VirtualToolIndex::from_raw((current_tool_.to_raw() + 1) % VirtualToolIndex::count);
        }
    };

    if (async_job_.state() == AsyncJobBase::State::finished) {
        // Call the async job finish callback
        if (auto f = async_job_.result()) {
            f({
                .tracker = *this,
                .tool_data = tool_data_[current_tool_],
                .tool = current_tool_,
            });
        }

        // Return to idle state so that the callback is not called multiple times
        async_job_.discard();

        // Trigger the next_tool_guard.
        // We don't want to get infinitely stuck on a single tool if an IO operation fails.
        return;
    }

    auto &tool_data = tool_data_[current_tool_];

    const ToolTag::UIDHash new_assigned_tag = ToolTag::for_tool_assigned(current_tool_).transform(&ToolTag::uid_hash).value_or(ToolTag::no_tag_hash);
    if (tool_data.assigned_tag != new_assigned_tag) {
        tool_data = ToolData {
            .base_extruded_distance_mm = filament_tracker().get_extruded_distance(current_tool_),
            .assigned_tag = new_assigned_tag,
            .init_pending = true,
            .unrecoverable_error = (new_assigned_tag == ToolTag::no_tag_hash),
        };
    }

    if (tool_data.assigned_tag == ToolTag::no_tag_hash || tool_data.unrecoverable_error) {
        return;
    }

    const ToolTag tool_tag { current_tool_, tool_data.assigned_tag };

    if (ToolTag::for_tool_ephemeral(current_tool_) != tool_tag) {
        // Tag is not present, no point in trying to read/write anything
        return;
    }

    if (tool_data.init_pending) {
        // We need to first some data from the tag to be able to track
        async_job_.issue([tag = tool_tag](AsyncJobExecutionControl &ctrl, AsyncJobFinishCallback &result) {
            result = tool_init_async(ctrl, tag);
        });

    } else if (tool_data.write_pending) {
        debug_assert(!std::isnan(tool_data.g_per_mm));
        const auto &args = write_consumption_args_.emplace(WriteConsumptionArgs {
            .tag = tool_tag,
            .extruded_distance_delta_mm = uncommited_consumption_mm_nolock(current_tool_),
            .g_per_mm = tool_data.g_per_mm,
        });

        async_job_.issue([&args](AsyncJobExecutionControl &ctrl, AsyncJobFinishCallback &result) {
            result = write_consumption_async(ctrl, args);
        });
    }
}

void FilamentUsageTracker::cannot_track_finish_cb(const AsyncJobFinishCallbackArgs &args) {
    auto &tool_data = args.tool_data;

    // Further disregard this tag
    tool_data.unrecoverable_error = true;
    tool_data.init_pending = false;
    tool_data.write_pending = false;
    tool_data.warn_if_next_write_fails = false;

    marlin_server::set_warning(WarningType::OpenPrintTagCannotTrack);
}

void FilamentUsageTracker::retry_finish_cb(const AsyncJobFinishCallbackArgs &args) {
    auto &tool_data = args.tool_data;

    // Show the warning even if this is not a write. If we fail init before write, it's also not good.
    if (tool_data.warn_if_next_write_fails) {
        marlin_server::set_warning(WarningType::OpenPrintTagUsageWriteFailed);

        // Clear ONLY if we've shown the warning - this might NOT be a write command
        // Without showing the warning, the flag should only reset on a successfull write
        tool_data.warn_if_next_write_fails = false;
    }
}

FilamentUsageTracker::AsyncJobFinishCallback FilamentUsageTracker::tool_init_async([[maybe_unused]] AsyncJobExecutionControl &ctrl, ToolTag tag) {
    MultiReadFieldRequest<AmountsInfo::Requirements {}> req { tag };
    req.issue();

    // Wait for the request to be finished
    while (!req.finished()) {
        freertos::delay(10);
    }

    AmountsInfo amounts(req);

    // Check possible errors
    for (Request *req : req.requests()) {
        if (!req->has_error()) {
            continue;
        }

        log_error(OpenPrintTag, "tool_init read error field=%i err=%i", (int)static_cast<ReadFieldRequestBase *>(req)->field(), (int)req->error());

        using Error = Request::Error;
        switch (req->error()) {

        case Error::data_too_big:
        case Error::wrong_field_type:
        case Error::field_not_present:
            // Standard errors that will be handled further down the line, can't do much about them
            break;

        case Error::other:
            // Something kinda failed, retrying might help
            return retry_finish_cb;

        case Error::region_corrupt:
        case Error::tag_invalid:
            // Unrecoverable problem, report that we won't be able to track filament usage
            return cannot_track_finish_cb;

        case Error::not_implemented:
        case Error::write_protected:
        case Error::radio_disabled:
        case Error::_cnt:
            bsod_unreachable();
        }
    }

    if (!amounts.full_length_mm.has_value() || !amounts.full_weight_g.has_value() || !amounts.remaining_weight_g.has_value()) {
        return cannot_track_finish_cb;
    }

    const float g_per_mm = *amounts.full_weight_g / *amounts.full_length_mm;
    const float remaining_g = *amounts.remaining_weight_g;

    return [g_per_mm, remaining_g](const AsyncJobFinishCallbackArgs &args) {
        auto &tool_data = args.tool_data;

        tool_data.g_per_mm = g_per_mm;
        tool_data.base_remaining_filament_g = remaining_g;
        tool_data.init_pending = false;

        // In case there is any untracked filament usage, write it immediately
        args.tracker.flush_nolock({ .tools = args.tool });
    };
}

FilamentUsageTracker::AsyncJobFinishCallback FilamentUsageTracker::write_consumption_async([[maybe_unused]] AsyncJobExecutionControl &ctrl, const WriteConsumptionArgs &args) {
    float old_consumed_weight;
    {
        ReadFieldRequest<AuxField::consumed_weight> req { args.tag };
        req.issue();

        // Wait for the request to be finished
        while (!req.finished()) {
            freertos::delay(10);
        }

        if (req.has_error()) {
            log_error(OpenPrintTag, "write_consumption read err=%i", (int)req.error());

            using Error = Request::Error;
            switch (req.error()) {

            case Error::field_not_present:
                // This is okay, defaults to 0
                break;

            case Error::other:
                // Something kinda failed, retrying might help
                return retry_finish_cb;

            case Error::data_too_big:
            case Error::region_corrupt:
            case Error::tag_invalid:
            case Error::wrong_field_type:
                return cannot_track_finish_cb;

            case Error::write_protected:
            case Error::not_implemented:
            case Error::radio_disabled:
            case Error::_cnt:
                // These do not make sense for the request
                bsod_unreachable();
            }
        }

        old_consumed_weight = req.result().value_or(0);
    }

    const float consumed_diff = args.extruded_distance_delta_mm * args.g_per_mm;
    const float new_consumed_weight = old_consumed_weight + consumed_diff;

    {
        WriteFieldRequest<AuxField::consumed_weight> req { args.tag, new_consumed_weight };
        req.issue();

        // Wait for the request to be finished
        while (!req.finished()) {
            freertos::delay(10);
        }

        if (req.has_error()) {
            log_error(OpenPrintTag, "write_consumption write err=%i", (int)req.error());

            using Error = Request::Error;
            switch (req.error()) {

            case Error::other:
                // Something kinda failed, retrying might help
                return retry_finish_cb;

            case Error::data_too_big:
            case Error::region_corrupt:
            case Error::tag_invalid:
            case Error::write_protected:
                return cannot_track_finish_cb;

            case Error::not_implemented:
            case Error::wrong_field_type:
            case Error::field_not_present:
            case Error::radio_disabled:
            case Error::_cnt:
                // These do not make sense for the request
                bsod_unreachable();
            }
        }
    }

    return [extruded = args.extruded_distance_delta_mm, consumed_diff](const AsyncJobFinishCallbackArgs &args) {
        auto &tool_data = args.tool_data;

        // Let the tracker know that we've succesfully written this amount of filament usage
        tool_data.base_extruded_distance_mm += extruded;
        tool_data.base_remaining_filament_g -= consumed_diff;
        tool_data.write_pending = false;
        tool_data.warn_if_next_write_fails = false;

        marlin_server::clear_warning(WarningType::OpenPrintTagUsageWriteFailed);
    };
}

} // namespace buddy::openprinttag
