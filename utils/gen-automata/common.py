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
            struct automata::StrPath paths_array[] = {
            """
        ]
        output.extend(["{ \"%s\" }," % p for p in self.__paths])
        output.append("""
            };
            struct automata::Transition transitions_array[] = {
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
            else:
                assert False
            output.append(
                "{ %d, automata::LabelType::%s, %s, %s }," %
                (target, ltype, label,
                 str(fallthrough).lower()))  # TODO: C-style escaping!
        output.append("""
            };
            struct automata::State states_array[] = {
            """)
        for (first_transition, emit_enter, emit_leave, path, has_path,
             path_nocase) in self.__states:
            output.append("{ %d, %s, %s, %d, %s, %s }," %
                          (first_transition, str(emit_enter).lower(),
                           str(emit_leave).lower(), path,
                           str(has_path).lower(), str(path_nocase).lower()))
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

    def compress(self):
        pass  # TODO

    def compile(self, namespace):
        self.compress()

        for (i, state) in enumerate(self.__states):
            state.set_id(i)

        names = {}
        transitions = []
        states = []
        paths = []

        for state in self.__states:
            first_transition = len(transitions)
            (emit_enter, emit_leave) = state.emit_marks()
            path_idx = 0
            has_path = False
            (path, path_nocase) = state.path()
            if path:
                # TODO: Try finding existing one
                path_idx = len(paths)
                paths.append(path)
                has_path = True
            for transition in state.transitions():
                target = transition.target_state_number()
                (label, label_type) = transition.label()
                transitions.append((target, label_type, label))
            name = state.name()
            if name:
                assert name not in names
                names[name] = state.id()
            states.append((first_transition, emit_enter, emit_leave, path_idx,
                           has_path, path_nocase))
        states.append((len(transitions), False, False, 0, False, False))

        return Compiled(namespace, names, paths, transitions, states)


class State:
    def __init__(self, name=None):
        self.__transitions = []
        self.__name = name
        self.__id = None  # Not known yet
        self.__emit_leave = False
        self.__emit_enter = False
        self.__path = None
        self.__path_nocase = False

    def add_transition(self, label, label_type, target):
        transition = Transition(label, label_type, target)
        self.__transitions.append(transition)
        return transition

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

    def path(self):
        return (self.__path, self.__path_nocase)

    def name(self):
        return self.__name


class Transition:
    def __init__(self, label, label_type, target):
        self.__label = label
        self.__label_type = label_type
        self.__target_state = target

    def target_state(self):
        return self.__target_state

    def target_state_number(self):
        return self.target_state().id()

    def label(self):
        return (self.__label, self.__label_type)
