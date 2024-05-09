"""
Utilities for generating parsing automata.

This closely cooperates with the lib/WUI/automata/core.* files on the C++ side.
This generates the raw automaton tables the algorithm there uses.

We provide both higher level constructs and leaves (eg. a trie, or connecting
multiple automata together in various ways) (TODO: implemented on-demand, as
needed) and low-level compilation/optimization of the tables. The hard part
about writing an automaton is not thinking about how it should look like, but:
* Dealing with the growing complexity as smaller automata join to form bigger
  ones.
* Indexing hell when in the raw-table form.

So the aim of this is to deal with both in somewhat usable way. As it is a
single-use tool, it is not always polished.

The structures here closely correspond to the C++ counterpart, except they are
linked in memory by pointers and are easier to grow piece by piece.
"""
from enum import Enum, auto


class LabelType(Enum):
    """
    The type of label.

    Corresponds to the one on the C++ side (has the same meaning and names).
    """
    Char = auto()
    CharNoCase = auto()
    Special = auto()
    Path = auto()


class Compiled:
    """
    A compiled form of an automaton.

    This holds the automaton in already-tabular form and can be asked to output
    it in whatever format needed. We currently support C++ file+header (two
    different outputs, each holding a part). We could support other languages
    or some form of graph (dot?).

    Obtained from Automaton.compile()
    """

    def __init__(self, namespace, names, paths, transitions, states):
        self.__namespace = namespace
        self.__names = names
        self.__paths = paths
        self.__transitions = transitions
        self.__states = states
        # TODO: Check limits (unlikely to hit, but...)

    def cpp_header(self):
        return f"""
            #pragma once
            namespace automata {{
                struct StrPath;
                struct Transition;
                struct State;
            }}
            namespace {self.__namespace} {{
                enum Names {{
                    { ','.join(' = '.join([name, str(value)]) for name, value in self.__names.items()) }
                }};
                extern const automata::StrPath *paths;
                extern const automata::Transition *transitions;
                extern const automata::State *states;
            }}
        """

    def cpp_file(self):
        output = [
            """
            #include <automata/core.h>
            namespace {
            constexpr const struct automata::StrPath paths_array[] = {
            """
        ]
        output.extend(["{ \"%s\" }," % p for p in self.__paths])
        output.append("""
            };
            constexpr const struct automata::Transition transitions_array[] = {
            """)
        for (target, ltype, label, fallthrough) in self.__transitions:
            if ltype == LabelType.Char:
                ltype = "Char"
                label = "'%s'" % label
            elif ltype == LabelType.CharNoCase:
                ltype = "CharNoCase"
                label = "'%s'" % label
            elif ltype == LabelType.Special:
                ltype = "Special"
                label = "automata::SpecialLabel::%s" % label
            elif ltype == LabelType.Path:
                ltype = "Path"
                label = self.__paths.index(label)
            else:
                assert False
            output.append(
                "{ %d, automata::LabelType::%s, %s, %s }," %
                (target, ltype, label,
                 str(fallthrough).lower()))  # TODO: C-style escaping!
        output.append("""
            };
            constexpr const struct automata::State states_array[] = {
            """)
        for (first_transition, emit_enter, emit_leave) in self.__states:
            output.append("{ %d, %s, %s }," %
                          (first_transition, str(emit_enter).lower(),
                           str(emit_leave).lower()))
        output.append(f"""
            }};
            }}
            namespace {self.__namespace} {{

            const automata::StrPath *paths = paths_array;
            const automata::Transition *transitions = transitions_array;
            const automata::State *states = states_array;
            }}
            """)
        return "\n".join(output)


class Automaton:
    """
    The top-level automaton.

    It is "born" with a single starting state.

    You can add states (and add transitions to the states) and you can compile
    it eventually.
    """

    def __init__(self):
        self.__states = []
        self.add_state()

    def start(self):
        return self.__states[0]

    def add_state(self, name=None):
        state = State(name)
        self.__states.append(state)
        return state

    def join(self, state, another):
        """
        Connects this automaton with another by identifying the given state
        with the start state of the another.

        Transitions from state and from the start state of the another are
        concatenated.

        This does a shallow copy of the content of another. All the
        ramifications of joining with one automaton multiple times or using the
        another after it has been joined are not thought through and would
        probably be surprising.

        It is however supported to keep links to states from both automata
        except the start of the another (the identity of the newly joined state
        is `state`).
        """
        start = another.start()
        for trans in another.transitions_to(start):
            trans.set_target(state)
        self.__states.extend(another.__states[1:])
        state.join(start)

    def join_transition(self, state, another, fallthrough=False):
        """
        Similar to join, but doesn't identify the given states. It
        places catch-all transition between them. Optionally, it can be a
        fallthrough one.

        This produces a slightly bigger automaton, but keeps the
        transitions of the states separate (also names, events, etc).
        """
        start = another.start()
        self.__states.extend(another.__states)
        state.add_transition("All", LabelType.Special, start, fallthrough)

    def compile(self, namespace):
        for (i, state) in enumerate(self.__states):
            state.set_id(i)

        names = {}
        transitions = []
        states = []
        paths = []

        for state in self.__states:
            first_transition = len(transitions)
            (emit_enter, emit_leave) = state.emit_marks()
            for transition in state.transitions():
                target = transition.target_state_number()
                (label, label_type) = transition.label()
                if label_type == LabelType.Path:
                    if not label in paths:
                        paths.append(label)
                fallthrough = transition.fallthrough()
                transitions.append((target, label_type, label, fallthrough))
            name = state.name()
            if name:
                assert name not in names
                names[name] = state.id()
            states.append((first_transition, emit_enter, emit_leave))
        # Sentinel state (not reachable, but the last one needs it to know how
        # many transitions it has.
        states.append((len(transitions), False, False))

        return Compiled(namespace, names, paths, transitions, states)

    def transitions_to(self, target):
        for state in self.__states:
            for trans in state.transitions():
                if target == trans.target_state():
                    yield trans


class State:
    def __init__(self, name=None):
        self.__transitions = []
        self.__name = name
        self.__id = None  # Not known yet
        self.__emit_leave = False
        self.__emit_enter = False

    def add_transition(self, label, label_type, target, fallthrough=False):
        transition = Transition(label, label_type, target, fallthrough)
        self.__transitions.append(transition)
        return transition

    def add_fallback(self, target, fallthrough=False):
        self.add_transition("All", LabelType.Special, target, fallthrough)

    def transitions(self):
        return self.__transitions

    def id(self):
        return self.__id

    def set_id(self, id):
        self.__id = id

    def emit_marks(self):
        return (self.__emit_enter, self.__emit_leave)

    def mark_enter(self):
        self.__emit_enter = True

    def mark_leave(self):
        self.__emit_leave = True

    def name(self):
        return self.__name

    def set_name(self, name):
        self.__name = name

    def find_next_state(self, label):
        """
        Finds a Char or CharNoCase transition with the given label or None if
        it doesn't exist. Returns the state it leads to.
        """
        for t in self.__transitions:
            (l, lt) = t.label()
            if lt == LabelType.Char and l == label:
                return t.target_state()
            elif lt == LabelType.CharNoCase and l == label.lower():
                return t.target_state()
        else:
            return None

    def loop(self, label, label_type):
        self.add_transition(label, label_type, self)

    def loop_fallback(self):
        self.loop("All", LabelType.Special)

    def join(self, another):
        self.__emit_enter |= another.__emit_enter
        self.__emit_leave |= another.__emit_leave
        self.__name = self.__name or another.__name
        self.__transitions.extend(another.__transitions)


class Transition:
    def __init__(self, label, label_type, target, fallthrough=False):
        if label_type == LabelType.CharNoCase:
            label = label.lower()
        self.__label = label
        self.__label_type = label_type
        self.__target_state = target
        self.__fallthrough = fallthrough

    def set_target(self, target):
        self.__target_state = target

    def target_state(self):
        return self.__target_state

    def target_state_number(self):
        return self.target_state().id()

    def label(self):
        return (self.__label, self.__label_type)

    def fallthrough(self):
        return self.__fallthrough
