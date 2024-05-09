from common import Automaton, LabelType

# TODO: This code is a bit ad-hoc and needs some redesign.


def trie(words, nocase=True):
    """
    Builds a trie automaton

    Recognizes bunch of "words".

    Returns the automaton, mapping of words to their terminal states and all
    states that might want to have a transition to "unknown" state added.
    """
    # TODO: Optimize by paths!
    auto = Automaton()
    start = auto.start()
    add_unknowns = [start]
    if nocase:
        label_type = LabelType.CharNoCase
    else:
        label_type = LabelType.Char
    terminals = {}
    for word in words:
        state = start
        for letter in word:
            old = state
            state = old.find_next_state(letter)
            if not state:
                state = auto.add_state()
                old.add_transition(letter, label_type, state)
                add_unknowns.append(state)
        terminals[word] = state
    return auto, terminals, add_unknowns


def keywords(words,
             term_label,
             term_label_type,
             unknown=True,
             nocase=True,
             allow_empty=False):
    """
    Generates an automata that accepts bunch of keywords terminated by a
    leaving transition. The idea is that in HTTP or other places, there are
    many places where the keywords are known in advance and they are terminated
    by something (colon, space...). Think the HTTP method.

    Returns the automaton and the final state.

    Parameters:
    - words: string->state name dict.
    - term_label: The label to "terminate" (eg. the space)
    - term_label_type: The type of label (one of LabelType)
    - unknown: Accept unknown ones too as a fallback. If true, it
      simply generates the code. If string, also used as the name for
      that state.
    - nocase: Do it case insensitively.
    - allow_empty: Allow for empty keyword. Effective only with has_unknown.
    """
    auto, terminals, add_unknowns = trie(words, nocase)
    start = auto.start()
    final = auto.add_state()
    final.mark_enter()
    for word in words:
        terminals[word].set_name(words[word])
        terminals[word].add_transition(term_label, term_label_type, final)
    if unknown:
        unknown_state = auto.add_state()
        if type(unknown) == str:
            unknown_state.set_name(unknown)
        unknown_state.add_transition(term_label, term_label_type, final)
        unknown_state.loop_fallback()
        if allow_empty:
            start.add_fallback(unknown_state)
        for s in add_unknowns:
            s.add_fallback(unknown_state)
    return auto, final


def read_until(term_label, term_label_type, inner_name=None):
    """
    Read a string until a terminator is found. Eg. read until space.

    If inner_name is set, an event on each read (not terminator)
    letter is emitted with the given name for entered state.

    Returns the automaton and the final state.
    """
    auto = Automaton()
    start = auto.start()
    if inner_name:
        start.set_name(inner_name)
        start.mark_enter()

    final = auto.add_state()
    start.add_transition(term_label, term_label_type, final)
    start.loop_fallback()
    return auto, final


def newline():
    """
    Automaton accepting arbitrary newline.

    While most network protocols want to work with CRLF, there are
    sometimes misguided implementations that send some other form of
    newline. And it's easier to accept "any" newline for us anyway.
    """
    auto = Automaton()
    start = auto.start()
    final = auto.add_state()
    # Direct LF
    start.add_transition('\\n', LabelType.Char, final)
    # First CR
    cr = auto.add_state()
    start.add_transition('\\r', LabelType.Char, cr)
    # CR->LF
    cr.add_transition('\\n', LabelType.Char, final)
    # CR->something else->don't consume the something else and accept
    cr.add_fallback(final, fallthrough=True)
    return auto, final


def constant(s):
    """
    Automaton accepting/consuming a string constant.
    """
    assert len(s) >= 2  # Not implemented yet
    auto = Automaton()
    start = auto.start()
    # We go through a single "real" transition, otherwise the
    # automaton acts a bit unpredictable when joining together with
    # others.
    mid = auto.add_state()
    final = auto.add_state()
    s = s.lower()
    mid.add_transition(s[1:], LabelType.Path, final)
    lt = LabelType.CharNoCase
    start.add_transition(s[0], lt, mid)
    return auto, final
