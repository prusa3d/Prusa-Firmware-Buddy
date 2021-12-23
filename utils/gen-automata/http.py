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
    return auto, end


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
        read_header, read_header_end = interested[header]
        auto.join_transition(separator, read_header)
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
    ignore_unknown_header, iuh_end = read_header_value(None)
    auto.join_transition(after_unknown, ignore_unknown_header)
    iuh_end.add_fallback(start, fallthrough=True)

    return auto, body


def request(interested):
    """
    Parser for the whole request, terminating by a body transition.
    """
    line, line_end = req_line()
    head, body = headers(interested)
    line.join_transition(line_end, head)

    return line, body


if __name__ == "__main__":
    want_headers = {
        'X-Api-Key': read_header_value('XApiKey'),
        'Content-Length': read_header_value('ContentLength'),
    }
    http, final = request(want_headers)
    compiled = http.compile("nhttp::parser::request")
    with open("http_req_automaton.h", "w") as header:
        header.write(compiled.cpp_header())
    with open("http_req_automaton.cpp", "w") as file:
        file.write(compiled.cpp_file())
