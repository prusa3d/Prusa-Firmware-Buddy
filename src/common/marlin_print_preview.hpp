/**
 * @file marlin_print_preview.hpp
 * @brief state machine for print preview
 */
#pragma once
#include "client_response.hpp"
#include <module/prusa/tool_mapper.hpp>
#include <module/prusa/spool_join.hpp>
#include <marlin_events.h>
#include <bitset>
#include "gcode_info.hpp"
#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>

/**
 * @brief Parent class handling changes of state
 * Automatically changes fsm
 */
class IPrintPreview {
public:
    enum class State {
        inactive,

        init,
        download_wait,
        loading,
        preview_wait_user,

        unfinished_selftest_wait_user,

        new_firmware_available_wait_user,

        wrong_printer_wait_user,
        wrong_printer_wait_user_abort,

        filament_not_inserted_wait_user,
        filament_not_inserted_load,

#if HAS_MMU2()
        mmu_filament_inserted_wait_user,
        mmu_filament_inserted_unload,
#endif

#if HAS_MMU2() || HAS_TOOLCHANGER()
        tools_mapping_wait_user,
#endif

        wrong_filament_wait_user,
        wrong_filament_change,

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
    static constexpr uint32_t new_firmware_timeout_ms { 30000 };
public:
    enum class Result {
        Wait,
        // Showing the image and asking if print.
        Image,
        // Asking the user something (wrong printer, etc).
        Questions,
#if HAS_TOOLCHANGER() || HAS_MMU2()
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

    /**
     * @brief Checks whether the given physical extruder has corrent filament type for the print. Parametrized with getter to be callable without global tool_mapper/spool_join being in a valid state
     *
     * @param physical_extruder extruder to be checked
     * @param no_gcode_value Return value of gcode_extruder_getter if physical_extruder doesn't print anything
     * @param gcode_extruder_getter Call to get assigned gcode extruder to physical_extruder
     */
    static bool check_correct_filament_type(uint8_t physical_extruder, uint8_t no_gcode_value, std::function<uint8_t(uint8_t)> gcode_extruder_getter);

    /**
     * @brief Checks whether given physical extruder needs to have a filament loaded -> if it's used in a print and not loaded, then it needs to load. nt. Parametrized with getter to be callable without global tool_mapper/spool_join being in a valid state
     *
     * @param physical_extruder extruder to be checked
     * @param no_gcode_value Return value of gcode_extruder_getter if physical_extruder doesn't print anything
     * @param gcode_extruder_getter Call to get assigned gcode extruder to physical_extruder
     */
    static bool check_extruder_need_filament_load(uint8_t physical_extruder, uint8_t no_gcode_value, std::function<uint8_t(uint8_t)> gcode_extruder_getter);

#if ENABLED(PRUSA_SPOOL_JOIN) && ENABLED(PRUSA_TOOL_MAPPING)
    struct ToolsMappingValidty {
        std::bitset<EXTRUDERS> unassigned_gcodes {};
        std::bitset<EXTRUDERS> mismatched_filaments {};
        std::bitset<EXTRUDERS> mismatched_nozzles {};
        std::bitset<EXTRUDERS> unloaded_tools {};

        [[nodiscard]] bool all_ok() const;
    };
    [[nodiscard]] static ToolsMappingValidty check_tools_mapping_validity(const ToolMapper &mapper, const SpoolJoin &joiner, const GCodeInfo &gcode);
#endif

private:
    uint32_t last_run = 0;
    uint32_t last_still_valid_check_ms = 0;

    marlin_server::PreviewSkipIfAble skip_if_able = marlin_server::PreviewSkipIfAble::no; ///< Whether to skip parts of preview when printing is started

    PrintPreview() = default;
    PrintPreview(const PrintPreview &) = delete;

    State stateFromFilamentPresence() const;
    State stateFromFilamentType() const;

    State stateFromSelftestCheck();
    State stateFromUpdateCheck();
    State stateFromPrinterCheck();
    Result stateToResult() const;
};
