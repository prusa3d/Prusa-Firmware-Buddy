#pragma once
#include "../../inc/MarlinConfigPre.h"

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "../../inc/MarlinConfig.h"
    #include "puppies/Dwarf.hpp"

using namespace buddy::puppies;

struct PrusaToolInfo {
    float kennel_x;
    float kennel_y;
};

class PrusaToolChanger {
public:
    static constexpr uint8_t MARLIN_NO_TOOL_PICKED = EXTRUDERS - 1;
    static constexpr auto PARKING_CURRENT_MA = 950;               ///< Higher motor current on the lock and unlock moves
    static constexpr auto PARKING_STALL_SENSITIVITY = 4;          ///< Stall sensitivity used when PARKING_CURRENT_MA is used
    static constexpr auto PARKING_FINAL_MAX_SPEED = 375.f;        ///< Maximum speed (mm/s) for parking
    static constexpr auto SLOW_ACCELERATION_MM_S2 = 400;          ///< Acceleration for parking and picking
    static constexpr auto FORCE_MOVE_MM_S = 30;                   ///< Not used here, feedrate for locking and unlocking the toolchange clamps
    static constexpr auto SLOW_MOVE_MM_S = 60;                    ///< Feedrate for tool picking and parking
    static constexpr auto TRAVEL_MOVE_MM_S = 400;                 ///< Feedrate for moves around kennel
    static constexpr uint32_t WAIT_TIME_TOOL_SELECT = 3000;       ///< Max wait for puppytask tool switch [ms], needs a lot of time if there is a hiccup in puppy communication
    static constexpr uint32_t WAIT_TIME_TOOL_PARKED_PICKED = 200; ///< Max wait for cheese to detect magnet [ms]
    static constexpr uint32_t WAIT_TIME_TOOL_PERIOD = 20;         ///< Check cheese magnet sensor this often [ms]
    static constexpr auto SAFE_Y_WITH_TOOL = 360.0f;
    static constexpr auto SAFE_Y_WITHOUT_TOOL = 425.0f;
    static constexpr auto KENNEL_OFFSET_X_MM = 82.0f;
    static constexpr auto KENNEL_DEFAULT_FIRST_X_MM = 25.0f;
    static constexpr auto KENNEL_DEFAULT_Y_MM = 455.0f;
    static constexpr auto KENNEL_INVALID_OFFSET_MM = 6; // TODO: Tighten this once 5mm smaller XLs are phased out

    PrusaToolChanger();

    /**
     * @brief Initialize toolchanger.
     * @param first_run should be true only the first time after boot
     * @return true on success, false on communication error
     */
    bool init(bool first_run);

    /**
     * @brief Change tool, park current and pick another.
     * Called from marlin task.
     * @param new_tool marlin index of a requested tool
     * @return true on success
     */
    bool tool_change(const uint8_t new_tool);

    /**
     * @brief Return the offset difference of the last *valid* toolchange
     * @return nozzle offset difference
     */
    xyz_pos_t last_tool_change_offset() { return tool_change_diff; }

    /**
     * @brief Plan a smooth unparking move towards destination
     * unpark_to() should only be called after pickup() in order to plan a smooth unpark move,
     * mirroring the park operation. Such move only makes sense if the toolhead hasn't stopped
     * for other operations, such as Z compensation.
     * @param destination
     */
    void unpark_to(const xy_pos_t &destination);

    /**
     * @brief Ger marlin tool index of a physically picked tool.
     * Can be called from anywhere.
     */
    uint8_t detect_tool_nr();

    /**
     * @brief Get currently selected dwarf or dwarfs[0] if none is selected
     * @return Dwarf instance
     */
    Dwarf &getActiveToolOrFirst();

    /**
     * @brief Get dwarf that is currently selected in marlin.
     * @return pointer to active dwarf or nullptr
     */
    Dwarf *get_marlin_picked_tool();

    Dwarf &getTool(uint8_t tool_index);
    const PrusaToolInfo &get_tool_info(const Dwarf &dwarf, bool check_calibrated = false) const;
    bool is_tool_info_valid(const Dwarf &dwarf, const PrusaToolInfo &info) const;
    bool is_tool_info_valid(const Dwarf &dwarf) const;
    void set_tool_info(const Dwarf &dwarf, const PrusaToolInfo &info);

    /**
     * @brief Update dwarf picked/parked and change active tool.
     * Called from puppy task.
     * @return true on success, false on communication error
     */
    bool update();

    /**
     * @brief Loop that checks toolchanger state.
     * Called directly from marlin server.
     * @param printing true if currently printing, to not start toolchange spontaneously
     */
    void loop(bool printing);

    /**
     * @brief Get binary mask of all enabled dwarfs.
     * @return high bits for dwarfs that is_enabled()
     */
    uint8_t get_enabled_mask();

    /**
     * @brief Get binary mask of all parked dwarfs.
     * @return high bits for dwarfs that is_enabled() && is_parked() && !is_picked()
     */
    uint8_t get_parked_mask();

    inline bool is_toolchanger_enabled() {
        return toolchanger_enabled;
    }

    inline bool is_tool_enabled(uint8_t tool) {
        assert(tool < dwarfs.size());
        return dwarfs[tool].is_enabled();
    }

    inline bool has_tool() const {
        return get_active_tool_nr() != MARLIN_NO_TOOL_PICKED;
    }

    [[nodiscard]] uint8_t get_active_tool_nr() const;

    [[nodiscard]] bool is_any_tool_active() const;
    [[nodiscard]] bool is_tool_active(uint8_t idx) const;
    [[nodiscard]] uint8_t get_num_enabled_tools() const;

    void load_tool_info();
    void save_tool_info();
    void load_tool_offsets();
    void save_tool_offsets();

    bool load_tool_info_from_usb();
    bool save_tool_info_to_usb();
    bool load_tool_offsets_from_usb();
    bool save_tool_offsets_to_usb();

    void expand_first_kennel_position(); // TODO: Is this still needed/wanted ?

    /**
     * @brief Request change of active dwarf.
     * This is only to be used in crash recovery, otherwise it could mess up marlin.
     * @param new_dwarf pointer to dwarf that will be selected as active, nullptr to select no dwarf
     * If it fails, it throws redscreen.
     */
    void request_active_switch(Dwarf *new_dwarf);

    /**
     * @brief Set feedrate to be used for toolchange.
     * @param feedrate feedrate [mm/s]
     */
    void set_feedrate(feedRate_t feedrate) { sampled_feedrate = feedrate; }

    /**
     * @brief Move to a XY position
     * @param x move to this x [mm]
     * @param y move to this y [mm]
     * @param feedrate use this feedrate [mm/s]
     */
    static void move(const float x, const float y, const feedRate_t feedrate);

private:
    /**
     * @brief Structure that sets something in its constructor and resets when destroyed.
     * Function set_state should be lambda and set something if state is true and reset if state is false.
     * It is used to clean up something on a return from a function.
     */
    struct ResetOnReturn {
        std::function<void(bool)> set_state;
        [[nodiscard]] ResetOnReturn(std::function<void(bool)> set_state)
            : set_state(set_state) { set_state(true); }
        ~ResetOnReturn() { set_state(false); }
    };

    bool toolchanger_enabled = false;

    std::atomic<bool> force_toolchange_gcode = false; ///< after reset force toolchange to init marlin tool variables
    std::atomic<bool> request_toolchange = false;     ///< when true, toolchange was requested and will be executed in puppytask
    std::atomic<Dwarf *> request_toolchange_dwarf;    ///< when request_toolchange=true, this specifies what tool will be changed to

    std::atomic<bool> block_tool_check = false;  ///< When true, block autodetect_active_tool() and set picked_dwarf as nullptr
    std::atomic<Dwarf *> picked_dwarf = nullptr; ///< what tool was physically detected as picked
    std::atomic<Dwarf *> active_dwarf = nullptr; ///< what tool is active in puppytask

    std::array<PrusaToolInfo, EXTRUDERS> tool_info;
    xyze_pos_t tool_change_diff; ///< Offset difference of the last tool change

    feedRate_t sampled_feedrate = NAN; ///< Feedrate to use on toolchange [mm/s], stored before marlin changes feedrate

    void ensure_safe_move();
    [[nodiscard]] bool can_move_safely();
    [[nodiscard]] PrusaToolInfo compute_synthetic_tool_info(const Dwarf &dwarf) const;

    bool autodetect_toolchanger_enabled();

    /**
     * @brief Get picked and parked states and detect which tool is active.
     * Modifies parked, picked and picked_dwarf.
     */
    void autodetect_active_tool();

    /**
     * @brief Pickup tool.
     * @param dwarf this tool
     * @param diff nozzle offset difference
     * @return true on success
     */
    [[nodiscard]] bool pickup(Dwarf &dwarf, const xyz_pos_t &diff);

    /**
     * @brief Park tool.
     * @param dwarf this tool
     * @return true on success
     */
    [[nodiscard]] bool park(Dwarf &dwarf);

    /**
     * @brief Compensate the Z offset by the specified amount
     */
    void z_shift(const float diff);

    /**
     * @brief Check if steps were skipped during parking.
     * If so, reset crash_s state and do homing.
     */
    void check_skipped_step();

    [[noreturn]] void toolchanger_error(const char *message) const;

    /**
     * @brief Wait until function returns true.
     * @param function wait for this to return true
     * @param timeout_ms maximal time to wait [ms]
     * @param period_ms check the function this often [ms]
     * @return true on success, false if timeout was reached
     */
    [[nodiscard]] bool wait(std::function<bool()> function, uint32_t timeout_ms, uint32_t period_ms = 1000);

    /**
     * @brief When printing and tool falls off, triggers toolchanger recovery sequence.
     * Called from marlin server loop() task.
     */
    void toolfall();

    /**
     * @brief When crash happens during toolchange, triggers toolchanger recovery sequence.
     * Called from tool_change().
     */
    void toolcrash();

    /**
     * @brief Force a selected tool to marlin.
     * @param dwarf force active_tool to be this dwarf, or nullptr for no tool
     */
    void force_marlin_picked_tool(Dwarf *dwarf);
};

extern PrusaToolChanger prusa_toolchanger;

#endif //PRUSA_TOOLCHANGER
