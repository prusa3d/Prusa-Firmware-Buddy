/**
 * \file
 * \brief Lightweight compile-time generated stream-parsing automata.
 *
 * We often need to parse network protocols using very little memory. As
 * strange as it seems, there probably aren't libraries matching our needs in
 * this area.
 *
 * We want to:
 * * Have small code footprint.
 * * Have very small state per-connection. That means not buffering eg. all the
 *   headers, etc.
 * * Be able to suspend and recover parsing anywhere (since data arrive in
 *   chunks and are split in arbitrary place).
 *
 * Lucky for us, a lot of network protocols are rather old and simple. That
 * means, it is easy to parse using quite simple automata. Furthermore, most
 * strings in the protocols (header names, method names, URLs of endpoints we
 * understand, ...) are known in advance and we can just ignore/not make
 * distinction between the others. That means the specific strings may be part
 * of that automaton too. Therefore, we can get away with just the state ID in
 * that automaton + few target variables for storing extracted values in
 * already parsed form (ID of a method or an endpoint in a numerical form, some
 * number).
 *
 * Therefore, the idea is we describe the parser in a python script using some
 * kind of grammer-like/parser-combinator-like DSL and let it generate an
 * automaton we'll use, subscribe to events it generates (entering/leaving
 * certain interesting states). We can feed the automaton char by char and keep
 * only the state around.
 *
 * # Technicalities
 *
 * For practical reasons, we don't store the automaton in the "pure
 * mathematical" definition, but in somewhat compressed form. For that reason,
 * a single transition in our case can be triggered by multiple different
 * characters. Also, it is possible to embed longer string "paths" that don't
 * have a separate state on each character.
 *
 * For technical reasons, paths are present in states, not in transitions and a
 * path forms kind of implicit transition in itself.
 *
 * The searching algorithm:
 *
 * * First tries if it can advance on the path. If it reaches the end of the
 *   path, it advances to a state with index current + 1 (the implicit
 *   transition).
 * * If there's no path or it can't be advanced (warning! this can "exit" the
 *   path in the middle of it), it tries all transitions available from the
 *   state. The first matching one is taken. Therefore, it is possible to have
 *   more specific transitions first, less specific ("fallbacks") later.
 * * If no transition is found, the given character is refused and the
 *   automaton doesn't move.
 * * Note that if a path is left "in the middle", it doesn't "return" back and
 *   try the transitions, they are consumed and lost forever. In case this
 *   greediness is not desired, the automaton needs to be "shaped" differently.
 *
 * If either the just left or just entered state is marked, an event with
 * details is emited and the caller can consume it. If both the entered and
 * left states are marked, only one event is generated.
 *
 * # Representation
 *
 * * The states and transitions are represented in the well-known two-array
 *   form of adjacency list. Each state points to the first transition leaving
 *   it, the next state forms a stopper for them (and there's one "dummy" state
 *   at the end).
 * * The generator must ensure no index is ever out of bound (eg. no invalid
 *   target states in transitions, etc).
 * * The whole representation is read-only and is generated as const tables
 *   during compile time. An Automaton class can be put on them to interpret
 *   them.
 * * It is assumed that the tables are static (available for the whole lifetime
 *   of the program), as are instances of the Automaton class.
 * * The data structures for the tables are somewhat packed to save space.
 * * We assume textual protocols here; it could be adapted for binary ones too,
 *   but we would save less bits in the packing. It is still assumed binary
 *   data may arrive over the wire (which would be violation of the protocol or
 *   in some
 * uninteresting place, but will not "blow up").
 */
#pragma once

#include <cstdbool>
#include <cstdint>
#include <optional>
#include <string_view>
#include <tuple>
#include <variant>

namespace automata {

/**
 * \brief Special checking for character category.
 *
 * Implemented by a function behind the scenes.
 */
enum SpecialLabel : uint8_t {
    /// Accepts all characters ("full fallback")
    All = 0,
    /// Accepts all whitespaces.
    Whitespace = 1,
    /// Horizontal whitespace (space or tab)
    HorizWhitespace = 2,
    /// 0-9
    Digit = 3,
};

/**
 * \brief Selects a mode in which a transition shall be tested to match.
 */
enum LabelType {
    /// Character equivalence.
    Char,
    /// Case-insensitive character equivalence.
    CharNoCase,
    /// Special function to check.
    Special,
};

using StateIdx = uint16_t;
using TransIdx = uint16_t;
using PathIdx = uint8_t;
using PathPos = uint8_t;
using OnPath = std::tuple<TransIdx, uint8_t>;

struct ActiveState {
    /// Which state we are at.
    StateIdx state;
    /// Index on the path leading from this state (if any).
    PathPos path;

    ActiveState(StateIdx idx)
        : state(idx)
        , path(0) {}
    ActiveState path_step() const {
        ActiveState copy = *this;
        copy.path++;
        return copy;
    }
};

/**
 * \brief Transition from one state to another.
 *
 * An edge in the graph terminology.
 */
struct Transition {
    /// Index of the destination state if the transition matches.
    StateIdx target_state : 12;
    /// How to check for a match.
    LabelType label_type : 2;
    /**
     * \brief Value for the matching.
     *
     * Depending on the label_type, this is:
     * * The character to be equal to.
     * * The lowercase version of the character in case of case-insensitive
     *   match (eg. already converted to lowercase).
     * * One of the SpecialLabel values to pick a function.
     */
    uint8_t label : 7;
    // Do not consume the character, offer it to the next state too.
    bool fallthrough : 1;
    /// Checks a match with the given byte.
    bool matches(uint8_t byte) const;
};

/**
 * \brief A state of the automaton.
 *
 * May contain 0-∞ transitions out and 0-1 paths leading out.
 *
 * Transitions are checked by single character, paths consume multiple bytes
 * (but can be abandoned at any place they don't match, in which case the
 * transitions will be considered). A path takes precedence over transitions.
 */
struct State {
    /**
     * \brief Index of the first transition leaving this state.
     *
     * Use the index of the next state in the array to know which one is (one
     * past) the last one. For this reason, there is/must be a single
     * sentinel/dummy state at the very end of the array _which is not
     * reachable_.
     */
    TransIdx first_transition : 14;
    // Emit an event when entering
    bool emit_enter : 1;
    // Emit an event when leaving.
    bool emit_leave : 1;
    /*
     * FIXME: Paths probably belong to transitions instead.
     *
     * They may more sense there. And adding transition offset (u8) to the
     * state won't make it bigger and we can afford to add few bits into
     * transition. But it'll make it a bit more complicated to match.
     */
    /**
     * \brief Description of a path.
     *
     * If has_path is true, this is an index into a third array of strings,
     * each one being a path.
     */
    PathIdx path : 7;
    bool has_path : 1;
    // Match the path case insensitively (must be already lower-case).
    bool path_nocase : 1;
};

/// Details of an event emitted.
struct Event {
    StateIdx leaving_state;
    StateIdx entering_state;
    bool triggered_by_leave : 1;
    bool triggered_by_enter : 1;
    // The last character that was fed there.
    uint8_t payload;
};

struct TransitionResult {
    ActiveState new_state;
    std::optional<Event> emit_event;
    /*
     * The caller is responsible for feeding the character into the
     * automaton once more. There was a transition, possibly with an
     * event generated, but the character was not consumed.
     */
    bool re_feed;
};

struct StrPath {
    /// Null-terminated.
    const char *value;
};

/**
 * \brief An automaton.
 *
 * This wraps the generated tables for better grasp.
 *
 * This is still something that would be constant/static and used over and over
 * again. It just ties the data tables together into a sane interface.
 */
class Automaton {
private:
    const StrPath *paths;
    const Transition *transitions;
    const State *states;
    std::optional<Event> gen_event(StateIdx old_state, StateIdx new_state, uint8_t payload) const;

public:
    constexpr Automaton(const StrPath *paths, const Transition *transitions, const State *states)
        : paths(paths)
        , transitions(transitions)
        , states(states) {}
    /**
     * \brief Starting state of the automaton.
     *
     * By convention, we start at the 0 state.
     */
    ActiveState start() const {
        return ActiveState(0);
    }
    /**
     * \brief Try performing a transition.
     *
     * If there's a way to transition from the old state given the byte, the
     * new state and possibly an event is returned.
     *
     * The state is in no way stored inside the automaton and must be managed
     * outside of it. A helper class is the Execution.
     */
    std::optional<TransitionResult> transition(ActiveState old, uint8_t byte) const;
};

enum class ExecutionControl {
    Continue,
    Stop,
    NoTransition,
};

/**
 * \brief Execution of an automaton.
 *
 * It is expected a user would inherit this class and override the `event`
 * method (to receive the events). Then, this'll keep the state of the
 * execution of the automaton when fed with characters.
 *
 * Note that the state here is copyable (you can "save" a position in the
 * automaton) and when an event desires a stop or a character is refused
 * because there's no transition, nothing special happens to this object and it
 * is possible to continue feeding other characters. This is more of an
 * artefact of the implementation than requirement, but may be useful.
 */
class Execution {
private:
    // TODO: Turn into a virtual method to save on space?
    const Automaton *automaton;

    ActiveState current_state;

protected:
    /**
     * \brief The event callback.
     *
     * The callback can consume the events and optionally signal a stop to the
     * automaton execution by the return value.
     */
    virtual ExecutionControl event(Event event) = 0;

public:
    Execution(const Automaton *automaton)
        : automaton(automaton)
        , current_state(automaton->start()) {}
    Execution(const Execution &other)
        : automaton(other.automaton)
        , current_state(other.current_state) {}
    Execution &operator=(const Execution &other) {
        automaton = other.automaton;
        current_state = other.current_state;
        return *this;
    }
    virtual ~Execution() = default;
    /**
     * \brief Feed a single byte.
     *
     * An event may be called.
     *
     * The result specifies if:
     * * The callback desires to stop the automaton.
     * * Or, there's no transition using this byte.
     */
    ExecutionControl feed(uint8_t byte);

    /**
     * \brief Feed as many bytes as possible, indicating how much was
     * consumed.
     *
     * Same as feed, but feeding everything it can. Feeding is stopped early if
     * there's no transition or if the inner event returns anything but
     * Continue.
     */
    std::tuple<ExecutionControl, size_t> consume(std::string_view data);
};

} // namespace automata
