// marlin_vars.h
#pragma once

#include "marlin_server_shared.h"
#include "cmsis_os.h"
#include "bsod.h"
#include <atomic>
#include "file_list_defs.h"
#include "fsm_types.hpp"
#include <cstring>
#include <charconv>
#include "inc/MarlinConfig.h"
#include <assert.h>

#if BOARD_IS_DWARF
    #error "You're trying to add marlin_vars to Dwarf. Don't!"
#endif /*BOARD_IS_DWARF*/

class MarlinVarsLockGuard {
public:
    [[nodiscard]] MarlinVarsLockGuard();
    ~MarlinVarsLockGuard();

private:
    MarlinVarsLockGuard &operator=(const MarlinVarsLockGuard &) = delete;
    MarlinVarsLockGuard(const MarlinVarsLockGuard &) = delete;
};

/**
 * @brief Thread-safe marlin variable. Uses std::atomic inside, but also does same checks that this is used properly.
 */
template <typename T>
class MarlinVariable {
public:
    MarlinVariable()
        : value((T)0) {
    }

    /**
     * @brief Assignemnt operator, only default task is allowed to write this variable.
     */
    T operator=(const T &other) {
        if (osThreadGetId() != marlin_server::server_task) {
            bsod("Write to marlin variable from non marlin thread");
        }
        value.store(other);
        return value.load();
    }

    /**
     * @brief Implicit conversion to underlying type, basically getter of this variable.
     */
    operator T() const { return get(); }

    /**
     * @brief Get current value atomically.
     */
    T get() const {
        return value.load();
    }

    /**
     * @brief Load this variable from string (for compatibility with marlin_server)
     */
    void from_string(const char *str_begin, const char *str_end) {
        if constexpr (std::is_floating_point<T>::value) {
            // our GCC doesn't support std::from_chars with floating point arguments :(
            value = strtof(str_begin, nullptr);
        } else {
            T new_value;
            auto [ptr, ec] = std::from_chars(str_begin, str_end, new_value);

            if (ec != std::errc {}) {
                bsod("marlin var: from string fail");
            }
            value = new_value;
        }
    }

    /**
     * @brief Call a callback with the current value.
     *
     * Calls the callback with the contained value (without copying it),
     * protected by the guard. Returns the result of the callback.
     *
     * Do not "exfiltrate" the pointer from the callback (it is not valid outside the callback).
     */
    template <class C>
    auto execute_with(C &&c) {
        const T temp = value.load();
        return c(temp);
    }

private:
    /// @brief  Underlying atomic variable
    std::atomic<T> value;
    static_assert(std::atomic<T>::is_always_lock_free, "MarlinVariable needs to be lock free, no structures allowed!");

    // disable copy operators
    MarlinVariable &operator=(const MarlinVariable &) = delete;
    MarlinVariable(const MarlinVariable &) = delete;
};

/**
 * @brief Marlin locked variable, with thread-safety. Access to it is guarded by marlin_vars mutex.
 * TODO: Merge with MarlinStringVariable
 */
template <typename T>
class MarlinVariableLocked {
public:
    /**
     * @brief Default constructor
     */
    MarlinVariableLocked() = default;

    /**
     * @brief Constructor with initial value
     */
    MarlinVariableLocked(const T &value)
        : value(value) {}

    /**
     * @brief Assign contained value
     * Using setter to assign value
     */
    void operator=(const T &other) {
        set(other);
    }

    /**
     * @brief Get current value
     * Protected by the guard.
     * @return T contained value
     */
    T get() const {
        auto guard = MarlinVarsLockGuard();
        return value;
    }

    /**
     * @brief Set current value
     * Protected by the guard.
     */
    void set(T value) {
        if (osThreadGetId() != marlin_server::server_task) {
            bsod("Write to marlin variable from non marlin thread");
        }
        auto guard = MarlinVarsLockGuard();
        this->value = value;
    }

    /**
     * @brief Call a callback with the current value.
     *
     * Calls the callback with the contained value (without copying it),
     * protected by the guard. Returns the result of the callback.
     *
     * Do not "exfiltrate" the pointer from the callback (it is not guaranteed
     * to be valid outside the callback).
     */
    template <class C>
    auto execute_with(C &&c) {
        auto guard = MarlinVarsLockGuard();
        return c(std::as_const(value));
    }

private:
    T value {};
};

/**
 * @brief Marlin string variable, with thread-safety. Access to it is guarded by marlin_vars mutex.
 *
 * @tparam LENGTH
 */
template <size_t LENGTH>
class MarlinVariableString {
public:
    MarlinVariableString()
        : value { 0 } {
    }

    /**
     * @brief Atomically copy this variable to other string.
     *
     * @param to
     * @param max_len
     */
    void copy_to(char *to, size_t max_len) const {
        auto guard = MarlinVarsLockGuard();
        copy_to(to, max_len, guard);
    }

    /**
     * @brief Copy contents of this variable to other string.
     *
     * You acquire lock yourself. Use this if you want to atomically sample multiple values.
     *
     * @param to
     * @param max_len
     */
    void copy_to(char *to, size_t max_len, MarlinVarsLockGuard &guard) const {
        (void)guard; // Lock argument is here just to make sure lock is acquired.
        strlcpy(to, value, max_len);
    }

    /**
     * @brief Call a callback with the current value.
     *
     * Calls the callback with the contained value (without copying it),
     * protected by the guard. Returns the result of the callback.
     *
     * Do not "exfiltrate" the pointer from the callback (it is not guaranteed
     * to be valid outside the callback).
     */
    template <class C>
    auto execute_with(C &&c) {
        auto guard = MarlinVarsLockGuard();
        return c(value);
    }

    /**
     * @brief Atomically change contents of this string
     *
     * @param from string to copy
     * @param max_len use max this number of characters (not counting '\0')
     */
    void set(const char *from, size_t max_len = LENGTH) {
        auto guard = MarlinVarsLockGuard();
        set(from, max_len, guard);
    }

    /**
     * @brief Atomically change contents of this string
     * You acquire lock yourself. Use this if you want to atomically sample multiple values.
     * @param from string to copy
     * @param max_len use max this number of characters (not counting '\0')
     * @param guard
     */
    void set(const char *from, size_t max_len, MarlinVarsLockGuard &guard) {
        (void)guard; // Lock argument is here just to make sure lock is acquired.
        strlcpy(value, from, std::min(max_len + 1, LENGTH));
    }

    /**
     * @brief Check if this string is equal to another.
     * @param with compare to this null-terminated string
     */
    bool equals(const char *with) const {
        auto guard = MarlinVarsLockGuard();
        return equals(with, guard);
    }

    /**
     * @brief Check if this string is equal to another.
     * You acquire lock yourself. Use this if you want to atomically sample multiple values.
     * @param with compare to this null-terminated string
     */
    bool equals(const char *with, MarlinVarsLockGuard &guard) const {
        (void)guard; // Lock argument is here just to make sure lock is acquired.
        return std::strncmp(value, with, LENGTH) == 0;
    }

    /**
     * @brief Get CONSTANT pointer to string, only call from default task.
     *
     * It is only possible to call this from default task, because only default task can write this variable.
     * Therefore its safe to read it without lock.
     */
    const char *get_ptr() const {
        // marlin thread can access pointer for read-only purposes without lock
        if (osThreadGetId() != marlin_server::server_task) {
            bsod("get_ptr");
        }
        return &value[0];
    }

    /**
     * @brief Get modifiable pointer to string, only call from default task and mutex has to be acquired beforehand.
     *
     * It is only possible to call this from default task, because only default task can write this variable.
     */
    char *get_modifiable_ptr(MarlinVarsLockGuard &guard) {
        (void)guard; // Lock argument is here just to make sure lock is acquired.

        // marlin server thread can get non-const pointer, but it has to hold mutex during writing, so only provide it when LockGuard is acquired
        if (osThreadGetId() != marlin_server::server_task) {
            bsod("get_ptr");
        }
        return &value[0];
    }

    constexpr size_t max_length() const {
        return LENGTH;
    }

private:
    char value[LENGTH];
};

enum {
    MARLIN_VAR_INDEX_X = 0,
    MARLIN_VAR_INDEX_Y = 1,
    MARLIN_VAR_INDEX_Z = 2,
    MARLIN_VAR_INDEX_E = 3,
};

class marlin_vars_t {
public:
    marlin_vars_t() {}
    void init();

    /**
     * @brief Printer position.
     * @note Not using structures to not lock Marlin too often.
     * Native coordinates are position of steppers or machine coordinates. Obtained from logical coordinates after applying tool and workspace offsets.
     * Logical coordinates are G-code coordinates compensating for workspace and tool offsets.
     * @todo When we have strong types for coordinates, we could give only native and user would convert coordinate systems on his own.
     * pos is taken from immediate stepper position.
     * curr_pos is taken from Marlin's current_position variable which is the target of current move before MBL is compensated.
     */
    MarlinVariable<float> native_pos[4]; ///< immediate position XYZE (native coordinates) [mm]
    MarlinVariable<float> logical_pos[4]; ///< immediate position XYZE (logical coordinates) [mm]
    MarlinVariable<float> native_curr_pos[4]; ///< current position XYZE (native coordinates) [mm]
    MarlinVariable<float> logical_curr_pos[4]; ///< current position XYZE (logical coordinates) [mm]

    MarlinVariable<float> temp_bed; // bed temperature [C]
    MarlinVariable<float> target_bed; // bed target temperature [C]
    MarlinVariable<float> z_offset; // probe z-offset [mm]
    MarlinVariable<float> travel_acceleration; // travel acceleration from planner
    MarlinVariable<uint32_t> print_duration; // print_job_timer.duration() [ms]
    MarlinVariable<uint32_t> time_to_end; // remaining print time (dumbly) calculated with speed [s]
    MarlinVariableLocked<time_t> print_start_time { marlin_server::TIMESTAMP_INVALID }; // Print start timestamp [s] since epoch
    MarlinVariableLocked<time_t> print_end_time { marlin_server::TIMESTAMP_INVALID }; // Estimated print end timestamp [s] since epoch

    MarlinVariableString<FILE_PATH_BUFFER_LEN> media_SFN_path;
    MarlinVariableString<FILE_NAME_BUFFER_LEN> media_LFN;
    MarlinVariable<marlin_server::State> print_state; // marlin_server.print_state

#if ENABLED(CANCEL_OBJECTS)
    MarlinVariable<uint32_t> cancel_object_mask; ///< Copy of mask of canceled objects
    MarlinVariable<int8_t> cancel_object_count; ///< Number of objects that can be canceled

    static constexpr size_t CANCEL_OBJECT_NAME_LEN = 32; ///< Maximal length of cancel_object_names strings
    static constexpr size_t CANCEL_OBJECTS_NAME_COUNT = 16; ///< Maximal number of cancel objects
    /// Names of cancelable objects
    MarlinVariableString<CANCEL_OBJECT_NAME_LEN> cancel_object_names[CANCEL_OBJECTS_NAME_COUNT];
#endif /*ENABLED(CANCEL_OBJECTS)*/

    // 2B base types
    MarlinVariable<uint16_t> print_speed; // printing speed factor [%]
    MarlinVariable<uint16_t> job_id; // print job id incremented at every print start(for connect)
    MarlinVariable<uint16_t> enabled_bedlet_mask; // enabled bedlet mask 1 - enabled, 0 disabled

    // 1B base types
    MarlinVariable<uint8_t> gqueue; // number of commands in gcode queue
    MarlinVariable<uint8_t> pqueue; // number of commands in planner queue
    MarlinVariable<uint8_t> sd_percent_done; // card.percentDone() [%]
    MarlinVariable<uint8_t> media_inserted; // media_is_inserted()
    MarlinVariable<uint8_t> fan_check_enabled; // fan_check [on/off]
    MarlinVariable<uint8_t> fs_autoload_enabled; // fs_autoload [on/off]
    MarlinVariable<uint8_t> mmu2_state; // 1 if MMU2 is on and works, 2 connecting, 0 otherwise - clients may use this variable to change behavior - with/without the MMU
    MarlinVariable<uint8_t> mmu2_finda; // FINDA pressed = 1, FINDA not pressed = 0 - shall be used as the main fsensor in case of mmu2State
    MarlinVariable<uint8_t> active_extruder; // See marlin's active_extruder. It will contain currently selected extruder (tool in case of XL, loaded filament nr in case of MMU2)

    // TODO: prints fans should be in extruder struct, but we are not able to control multiple print fans yet
    MarlinVariable<uint8_t> print_fan_speed; // print fan speed [0..255]

    // PER-Hotend variables (access via hotend(num) or active_hotend())
    struct Hotend {
        // nozzle
        MarlinVariable<float> temp_nozzle; // nozzle temperature [C]
        MarlinVariable<float> target_nozzle; // nozzle target temperature [C]
        MarlinVariable<float> display_nozzle; // nozzle temperature to display [C]

        // heatbreak
        MarlinVariable<float> temp_heatbreak; // heatbreak temperature [C]
        MarlinVariable<float> target_heatbreak; // heatbreak target temperature [C]
        MarlinVariable<uint16_t> heatbreak_fan_rpm; // Fans::heat_break(active_extruder).getActualRPM() [1/min]

        // others
        MarlinVariable<uint16_t> flow_factor; // flow factor [%]
        MarlinVariable<uint16_t> print_fan_rpm; // Fans::print(active_extruder).getActualRPM() [1/min]

        Hotend() {}
        // disable copy constructor
        Hotend(const Hotend &) = delete;
        Hotend &operator=(Hotend const &) = delete;
    };

    /// @brief  Reference to active extruder structure
    Hotend &active_hotend() {
        if constexpr (ENABLED(SINGLENOZZLE)) {
            // for MMU2 printers - hotend 0 is always active, no switching is possible
            return hotends[0];
        } else {
            // for toolchanger printers
            const uint8_t hotend = active_extruder.get();
            assert(hotend < hotends.max_size());
            return hotends[hotend];
        }
    }

    /**
     * @brief Reference to selected extruder (MARLIN_SERVER_CURRENT_TOOL means select current extruder )
     *
     * @param extruder
     * @return Extruder&
     */
    Hotend &hotend(uint8_t hotend) {
        if (hotend == marlin_server::CURRENT_TOOL) {
            return active_hotend();
        } else {
            assert(hotend < hotends.max_size());
            return hotends[hotend];
        }
    }

    struct FSMChange {
        fsm::Change q0_change;
        fsm::Change q1_change;

        FSMChange()
            : q0_change(fsm::QueueIndex::q0)
            , q1_change(fsm::QueueIndex::q1) {}
    };
    /**
     * @brief Get the last fsm state
     *
     * This is needed, because in Prusa link there is no way to use the callbacks as there is no place to call
     * marlin_client::loop periodically. Also for this to be stored in an atomic, we would need to make
     * atomic<uint64_t> work, which I was not able to do, if anyone knows how to, let me know.
     *
     * @return last change for both FSM queues
     */
    FSMChange get_last_fsm_change() {
        auto guard = MarlinVarsLockGuard();
        return last_fsm_state;
    }

    /**
     * @brief Set the last fsm state
     *
     * Can be called only from main task
     */
    void set_last_fsm_state(const fsm::Change &change) {
        if (osThreadGetId() != marlin_server::server_task) {
            bsod("set_last_fsm_state");
        }
        auto guard = MarlinVarsLockGuard();
        if (change.get_queue_index() == fsm::QueueIndex::q0) {
            last_fsm_state.q0_change = change;
        } else { /*(change.get_queue_index() == fsm::QueueIndex::q1)*/
            last_fsm_state.q1_change = change;
        }
    }

    void lock();
    void unlock();

private:
    osMutexDef(mutex); // Declare mutex
    osMutexId mutex_id; // Mutex ID
    std::atomic<osThreadId> current_mutex_owner; // current mutex owner -> to check for recursive locking
    std::array<Hotend, HOTENDS> hotends; // array of hotends (use hotend()/active_hotend() getter)
    FSMChange last_fsm_state; // last fsm state, used in connect and link

    // disable copy constructor
    marlin_vars_t(const marlin_vars_t &) = delete;
    marlin_vars_t &operator=(marlin_vars_t const &) = delete;
};

extern marlin_vars_t marlin_vars_instance;

inline constexpr marlin_vars_t *marlin_vars() {
    return (marlin_vars_t *)&marlin_vars_instance;
}
