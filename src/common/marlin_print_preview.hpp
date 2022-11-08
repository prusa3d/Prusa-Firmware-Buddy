/**
 * @file marlin_print_preview.hpp
 * @brief state machine for print preview
 */
#pragma once
#include "gcode_info.hpp"
#include "client_response.hpp"

/**
 * @brief Parent class handling changes of state
 * Automatically changes fsm
 */
class IPrintPreview {
public:
    enum class State {
        inactive,

        preview_wait_user,

        wrong_printer_wait_user,

        filament_not_inserted_wait_user,
        filament_not_inserted_load,

        mmu_filament_inserted_wait_user,
        mmu_filament_inserted_unload,

        wrong_filament_wait_user,
        wrong_filament_change,

        done
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

    static constexpr uint32_t max_run_period_ms = 50;

public:
    enum class Result {
        // Showing the image and asking if print.
        Image,
        // Asking the user something (wrong printer, etc).
        Questions,
        Abort,
        Print,
        Inactive
    };

    static PrintPreview &Instance() {
        static PrintPreview ret;
        return ret;
    }

    Result Loop();

    void Init(const char *path);
    void SkipIfAble() { skip_if_able = true; }
    void DontSkip() { skip_if_able = false; }

private:
    uint32_t last_run = 0;

    GCodeInfo::filament_buff filament_type;
    bool filament_described = false;
    bool valid_printer_settings = false;
    bool skip_if_able = false;

    PrintPreview() = default;
    PrintPreview(const PrintPreview &) = delete;

    State stateFromFilamentPresence() const;
    State stateFromFilamentType() const;

    State evaluateStateOnPrintClick();
    Result stateToResult() const;
};
