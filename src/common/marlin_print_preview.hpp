/**
 * @file marlin_print_preview.hpp
 * @brief state machine for print preview
 */
#pragma once
#include "client_response.hpp"
#include <module/prusa/tool_mapper.hpp>
#include <marlin_events.h>
#include <bitset>
#include "gcode_info.hpp"
#include <option/has_mmu2.h>
#include <option/has_tool_mapping.h>
#include <option/has_wastebin_fill_tracking.h>
#include <async_job/async_job.hpp>
#include <inplace_function.hpp>
#include <fsm/print_preview_phases.hpp>

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

/**
 * @brief Parent class handling changes of state
 * Automatically changes fsm
 */
class IPrintPreview {
public:
    enum class State : uint8_t {
        inactive,

        init,
        download_wait,
        loading,
        preview_wait_user,

        unfinished_selftest_wait_user,

        new_firmware_available_wait_user,

        gcode_invalid_wait_user,
        gcode_invalid_wait_user_abort,

#if HAS_WASTEBIN_FILL_TRACKING()
        wastebin_overfill_wait_user, ///< bin projected to overfill
        wastebin_emptying, ///< M1986 is parking and dropping the bed, before the empty-the-bin prompt
        wastebin_emptied_returning, ///< the bin has been emptied, M1986 is on its way back
#endif

        filament_not_inserted_wait_user,
        filament_not_inserted_load,

#if HAS_MMU2()
        mmu_filament_inserted_wait_user,
        mmu_filament_inserted_unload,
#endif

#if HAS_TOOL_MAPPING()
        tools_mapping_wait_user,
#endif

        wrong_filament_wait_user,
        wrong_filament_change,

#if HAS_E2EE_SUPPORT()
        untrusted_identity,
#endif
        file_error_wait_user, ///< Reports that something is wrong with the gcode file. The user is shwon an error message (and nothing is printed).

        checks_done,
        done,
    };

private:
    State state = State::inactive;
    std::optional<PhasesPrintPreview> phase = std::nullopt;

    static std::optional<PhasesPrintPreview> getCorrespondingPhase(State state);
    void setFsm(std::optional<PhasesPrintPreview> wantedPhase);

public:
    void ChangeState(State s);

    inline State GetState() const {
        return state;
    }
    Response GetResponse();
};

class PrintPreview : public IPrintPreview {

    static constexpr int32_t max_run_period_ms = 50;
    uint32_t new_firmware_open_ms { 0 };
    static constexpr uint32_t new_firmware_timeout_ms { 30'000 };

public:
    enum class Result : uint8_t {
        Wait,
        // Showing the image and asking if print.
        Image,
        // Asking the user something (wrong printer, etc).
        Questions,
#if HAS_TOOL_MAPPING()
        ToolsMapping,
#endif
        MarkStarted,
        Abort,
        Print,
        Inactive
    };

    static PrintPreview &Instance() {
        static PrintPreview ret;
        return ret;
    }

    /**
     * @brief Handles cleanup required by leaving tools_mapping screen.
     *
     * @param leaving_to_print Some cleanup is dependant whether the screen is left to go print or whether it's being left 'back home'
     */
    static void tools_mapping_cleanup(bool leaving_to_print = false);

    Result Loop();

    void Init();

    /**
     * @brief Configure whether to skip parts of preview when printing is started.
     * @param set skip these parts
     */
    inline void set_skip_if_able(marlin_server::PreviewSkipIfAble set) {
        skip_if_able = set;
    }

private:
    uint32_t last_run = 0;
    uint32_t last_still_valid_check_ms = 0;
    AsyncJob still_valid_check_job;

    marlin_server::PreviewSkipIfAble skip_if_able = marlin_server::PreviewSkipIfAble::no; ///< Whether to skip parts of preview when printing is started

    PrintPreview() = default;
    PrintPreview(const PrintPreview &) = delete;

    State stateFromFilamentPresence() const;
    State stateFromFilamentType() const;

    State stateFromSelftestCheck();
    State stateFromUpdateCheck();
    State stateFromPrinterCheck();
#if HAS_WASTEBIN_FILL_TRACKING()
    State stateFromWastebinCheck();
#endif
    Result stateToResult() const;
};
