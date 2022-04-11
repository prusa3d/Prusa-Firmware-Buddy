from common import Automaton, LabelType
from parts import constant, keywords, newline, read_until, trie


def methods():
    """
    Parser for the methods.

    Extend the list of methods as needed.
    """
    methods = {
        "GET": "MethodGet",
        "POST": "MethodPost",
        "PUT": "MethodPut",
        "DELETE": "MethodDelete",
        "HEAD": "MethodHead",
    }
    return keywords(methods,
                    "HorizWhitespace",
                    LabelType.Special,
                    unknown="MethodUnknown")


# TODO: Eventually, we may want to have a upfront-known list of URLs
# so we don't need to accumulate in a buffer.
def req_line():
    """
    Parser for the request line.
    """
    method, meth_spaces = methods()
    meth_spaces.loop("HorizWhitespace", LabelType.Special)
    url, url_spaces = read_until("HorizWhitespace", LabelType.Special, "Url")
    url_spaces.loop("HorizWhitespace", LabelType.Special)
    http, slash = constant("HTTP/", nocase=True)
    version = http.add_state("Version")
    version.mark_enter()
    slash.add_transition("Digit", LabelType.Special, version)
    version.loop(".", LabelType.Char)
    version.loop("Digit", LabelType.Special)
    line, end = newline()

    req_line = method
    req_line.join_transition(meth_spaces, url)
    req_line.join(url_spaces, http)
    req_line.join(version, line)
    return req_line, end


# Header values with internal parsing too?
def read_header_value(event_name):
    """
    Read and emit characters of a (possibly multi-line) header value.

    Note that the header is not "demangled" in any way, only the newlines are
    removed.
    """
    auto = Automaton()
    start = auto.start()
    start.set_name(event_name)
    start.mark_enter()
    line, end = newline()
    auto.join(start, line)
    start.loop_fallback()
    continuation = auto.add_state()
    continuation.loop("HorizWhitespace", LabelType.Special)
    continuation.add_fallback(start)
    end.add_transition("HorizWhitespace", LabelType.Special, continuation)
    return auto, end, False


# TODO: We should really come up with a way to replace one state of one
# automata with another whole automata, so we don't have to deal with the
# continuations, etc.
#
# Due to that, we simplify a bit and won't allow split header in the middle of
# the boundary or similar place.

# TODO: Generalize for a list of keywords, possibly with values, separated by
# _something_?


# TODO: This one might want to get more readable :-(
def read_boundary():
    """
    Read a boundary=value from a header.

    This is meant for content type header (we ignore the actual content type)
    """

    # States:
    # * We linger in read_until_colon first, then transition to waiting_word.
    # * The waiting_word is responsible to find the 'boundary=' word
    # * Then in_boundary accumulates the actual boundary
    # If we leave it, it means it is somehow unknown and we loop back to
    # waiting for another colon.
    #
    # And then to complicate things, there might be a newline that either means
    # end of the header or it may be a header continuation. In the latter case,
    # we need to distinguish the state of somewhere before or after colon.
    auto = Automaton()
    line, end = newline()

    read_until_colon = auto.start()

    waiting_word = auto.add_state()
    # Target od waiting_word's path
    equals = auto.add_state()

    in_boundary = auto.add_state("Boundary")
    equals.add_fallback(in_boundary)
    in_boundary.mark_enter()
    # Including newlines, yes - they'll be handled later.
    in_boundary.add_transition("Whitespace",
                               LabelType.Special,
                               read_until_colon,
                               fallthrough=True)
    in_boundary.add_transition(';',
                               LabelType.Char,
                               read_until_colon,
                               fallthrough=True)
    in_boundary.loop_fallback()
    waiting_word.set_path("boundary=", True)
    waiting_word.loop("HorizWhitespace", LabelType.Special)
    waiting_line, waiting_end = newline()
    auto.join(waiting_word, waiting_line)
    waiting_continuation = auto.add_state()
    waiting_continuation.loop("HorizWhitespace", LabelType.Special)
    waiting_continuation.add_fallback(waiting_word, fallthrough=True)
    waiting_word.add_fallback(read_until_colon, fallthrough=True)
    waiting_end.add_transition("HorizWhitespace", LabelType.Special,
                               waiting_continuation)
    waiting_end.add_fallback(end, fallthrough=True)

    read_until_colon.add_transition(';', LabelType.Char, waiting_word)

    auto.join(read_until_colon, line)
    continuation = auto.add_state()
    continuation.loop("HorizWhitespace", LabelType.Special)
    continuation.add_fallback(read_until_colon, fallthrough=True)
    end.add_transition("HorizWhitespace", LabelType.Special, continuation)

    read_until_colon.loop_fallback()

    return auto, end, False


def keyworded_header(keywords):
    """
    Header reader with keywords.

    Note that this assumes the keywords starts with different first letter!.
    """
    auto = Automaton()
    start = auto.start()
    start.loop('HorizWhitespace', LabelType.Special)

    terminals = []

    for kw in keywords:
        kw_start = auto.add_state()
        kw_start.mark_enter()
        start.add_transition(kw[0], LabelType.CharNoCase, kw_start)
        kw_start.set_path(kw[1:], nocase=True)  # Will lead to the next state
        end = auto.add_state(keywords[kw])
        end.mark_enter()
        terminals.append(kw_start)
        terminals.append(end)

    # Now handle all the rest by a header-parsing automaton that doesn't emit
    # any events.
    other, other_end, _ = read_header_value(None)
    fallback = other.start()
    auto.join_transition(start, other, fallthrough=True)

    # Whenever leaving any of our states, just move to the dummy header
    # collector that handles all the header continuations, header ends, etc.
    for state in terminals:
        state.add_fallback(fallback, fallthrough=True)

    return auto, other_end, True


def connection_header():
    """
    Parse a connection header.

    We want to recognize the Connection: close and Connection: keep-alive. In
    theory, the header may contain other things, but in practice it mostly
    doesn't happen. Doing it properly would be complicated (until we improve
    our automata-handling utilities) with very little gain, so we cheat a
    little bit.
    """
    return keyworded_header({
        'close': 'ConnectionClose',
        'keep-alive': 'ConnectionKeepAlive',
    })


def accept_header():
    """
    Looking at the accept header.

    We are cheating here a bit. We only want to know if the other side wants a
    application/json error messages and that doesn't list bunch of different
    ones. We pick the first one and check it is the one we want.
    """
    return keyworded_header({
        'application/json': 'AcceptJson',
    })


def headers(interested):
    """
    Automaton to read all the headers in a request followed by a transition to
    a body.

    This reads all the header names and values and recognizes the transition to
    the body (and emits that).

    Dictionary of recognized header names is to be passed in. The values are
    parsers for the header values (usually results of read_header_value, but
    could be replaced by something else).
    """
    # A trie of known headers
    auto, terminals, add_unknowns = trie(interested)
    start = auto.start()

    # If we see another CRLF after a header/start, it's the body (and
    # our terminal). That's not supposed to be in the trie, so its
    # disjoint transition.
    line, body = newline()
    body.set_name("Body")
    body.mark_enter()
    auto.join(start, line)

    # And similar thing for all the known headers too.
    for header in interested:
        term = terminals[header]
        term.loop("HorizWhitespace", LabelType.Special)
        separator = auto.add_state()
        separator.loop("HorizWhitespace", LabelType.Special)
        term.add_transition(':', LabelType.Char, separator)
        read_header, read_header_end, fallthrough = interested[header]
        auto.join_transition(separator, read_header, fallthrough=fallthrough)
        read_header_end.add_fallback(start, fallthrough=True)

    # Handling of unknown headers. We ignore few spaces, see a colon,
    # then maybe some more spaces and then there's a header value we
    # don't emit. And then back to the start for another header.
    unknown = auto.add_state()
    for u in add_unknowns:
        u.add_fallback(unknown)
    unknown.loop("HorizWhitespace", LabelType.Special)
    after_unknown = auto.add_state()
    unknown.add_transition(':', LabelType.Char, after_unknown)
    unknown.loop_fallback()
    after_unknown.loop("HorizWhitespace", LabelType.Special)
    ignore_unknown_header, iuh_end, _ = read_header_value(None)
    auto.join_transition(after_unknown, ignore_unknown_header)
    iuh_end.add_fallback(start, fallthrough=True)

    return auto, body


def request(interested):
    """
    Parser for the whole request, terminating by a body transition.
    """
    line, line_end = req_line()
    head, body = headers(interested)
    line.join_transition(line_end, head, fallthrough=True)

    return line, body


if __name__ == "__main__":
    want_headers = {
        'X-Api-Key': read_header_value('XApiKey'),
        'Content-Length': read_header_value('ContentLength'),
        'If-None-Match': read_header_value('IfNoneMatch'),
        'Content-Type': read_boundary(),
        'Connection': connection_header(),
        'Accept': accept_header(),
    }
    http, final = request(want_headers)
    compiled = http.compile("nhttp::parser::request")
    with open("http_req_automaton.h", "w") as header:
        header.write(compiled.cpp_header())
    with open("http_req_automaton.cpp", "w") as file:
        file.write(compiled.cpp_file())
