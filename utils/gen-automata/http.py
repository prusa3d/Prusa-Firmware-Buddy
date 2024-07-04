from os import sep
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


def http_version():
    """
    The HTTP/?.? version.
    """
    http, slash = constant("HTTP/")
    version = http.add_state("Version")
    version.mark_enter()
    slash.add_transition("Digit", LabelType.Special, version)
    version.loop(".", LabelType.Char)
    version.loop("Digit", LabelType.Special)
    return http, version


def content_type():
    """
    Content type decoder.
    """
    content_types = {
        'application/json': 'ApplicationJson',
        'text/x.gcode': 'TextGcodeDot',
        'text/x-gcode': 'TextGcodeDash',
    }
    tr, terminals, add_unknowns = trie(content_types)
    for t in terminals:
        terminals[t].mark_enter()
        terminals[t].set_name(content_types[t])
    # Eat spaces before the content type
    start = tr.start()
    start.loop('HorizWhitespace', LabelType.Special)

    # Now handle all the rest by a header-parsing automaton that doesn't emit
    # any events.
    other, other_end, _ = read_header_value(None)
    fallback = other.start()
    tr.join_transition(start, other, fallthrough=True)
    for unknown in add_unknowns:
        unknown.add_fallback(fallback, fallthrough=True)
    return tr, other_end, True


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
    http, version = http_version()
    line, end = newline()

    req_line = method
    req_line.join_transition(meth_spaces, url)
    req_line.join(url_spaces, http)
    req_line.join(version, line)
    return req_line, end


def resp_line():
    """
    Response line.
    """
    auto, version = http_version()
    version_space = auto.add_state()
    version.add_transition("HorizWhitespace", LabelType.Special, version_space)
    version_space.loop("HorizWhitespace", LabelType.Special)
    status = auto.add_state("StatusCode")
    status.mark_enter()
    status.loop("Digit", LabelType.Special)
    version_space.add_transition("Digit", LabelType.Special, status)
    rest = auto.add_state()
    status.add_transition("HorizWhitespace", LabelType.Special, rest)
    line, end = newline()
    auto.join(rest, line)
    rest.loop_fallback()

    return auto, end


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
    waiting_word.add_transition("boundary=", LabelType.Path, equals)
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


def keyworded_header(keywords, entry_name=None):
    """
    Header reader with keywords.

    Note that this assumes the keywords starts with different first letter!.
    """
    auto = Automaton()
    start = auto.start()
    start.loop('HorizWhitespace', LabelType.Special)
    if entry_name is not None:
        start.set_name(entry_name)
        start.mark_enter()

    terminals = []

    for kw in keywords:
        kw_start = auto.add_state()
        kw_start.mark_enter()
        start.add_transition(kw[0], LabelType.CharNoCase, kw_start)
        if len(kw) > 1:
            end = auto.add_state(keywords[kw])
            end.mark_enter()
            kw_start.add_transition(kw[1:], LabelType.Path, end)
            terminals.append(kw_start)
            terminals.append(end)
        else:
            kw_start.set_name(keywords[kw])
            terminals.append(kw_start)

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
    return keyworded_header(
        {
            'close': 'ConnectionClose',
            'keep-alive': 'ConnectionKeepAlive',
            'upgrade': 'ConnectionUpgrade',
        }, 'ConnectionHeader')


def upgrade_header():
    """
    Parse an upgrade header.

    We are looking for websocket only.
    """
    return keyworded_header({
        'websocket': 'UpgradeWebsocket',
    }, 'UpgradeHeader')


def ws_protocol_header():
    """
    The Sec-WebSocket-Protocol, looking for our prusa-connect protocol.
    """
    return keyworded_header({
        'prusa-connect': 'WsPrusaConnect',
    }, 'WsProtocol')


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


def content_encryption_mode_header():
    """
    Content encryption mode decoder.
    """
    encryption_modes = {
        'AES-CBC': 'ContentEncryptionModeCBC',
        'AES-CTR': 'ContentEncryptionModeCTR',
    }
    tr, terminals, add_unknowns = trie(encryption_modes)
    for t in terminals:
        terminals[t].mark_enter()
        terminals[t].set_name(encryption_modes[t])
    # Eat spaces before the encryption mode
    start = tr.start()
    start.loop('HorizWhitespace', LabelType.Special)

    # Now handle all the rest by a header-parsing automaton that doesn't emit
    # any events.
    other, other_end, _ = read_header_value(None)
    fallback = other.start()
    tr.join_transition(start, other, fallthrough=True)
    for unknown in add_unknowns:
        unknown.add_fallback(fallback, fallthrough=True)
    return tr, other_end, True


def print_after_upload_header():
    return keyworded_header({
        'true': 'PrintAfterUpload',
        '1': 'PrintAfterUploadNumeric',
        '?1': 'PrintAfterUploadRFC',
    })


def overwrite_file_header():
    return keyworded_header({
        '?1': 'OverwriteFile',
    })


def create_folder_header():
    return keyworded_header({
        '?1': 'CreateFolder',
    })


def auth_value(name):
    name_unquoted = f"{name}Unquoted" if name != None else None

    auto = Automaton()
    start = auto.start()
    quote = auto.add_state()
    value_quoted = auto.add_state()
    value_unquoted = auto.add_state()
    end = auto.add_state()

    start.loop("HorizWhitespace", LabelType.Special)
    start.add_transition('\"', LabelType.Char, quote)
    start.add_transition('Whitespace',
                         LabelType.Special,
                         end,
                         fallthrough=True)  # Vertical whitespace
    start.add_transition(',', LabelType.Char, end, fallthrough=True)
    start.add_transition('All', LabelType.Special, value_unquoted)

    quote.add_transition('\"', LabelType.Char, end)
    quote.add_transition('All', LabelType.Special, value_quoted)

    value_quoted.set_name(name)
    value_quoted.mark_enter()
    value_quoted.add_transition('\"', LabelType.Char, end)
    value_quoted.loop_fallback()

    value_unquoted.set_name(name_unquoted)
    value_unquoted.mark_enter()
    value_unquoted.add_transition('Whitespace',
                                  LabelType.Special,
                                  end,
                                  fallthrough=True)
    value_unquoted.add_transition(',', LabelType.Char, end, fallthrough=True)
    value_unquoted.loop_fallback()

    end.loop("HorizWhitespace", LabelType.Special)
    end.loop(',', LabelType.Char)

    return auto, end, True


def authorization_header():
    """
    Reads the digest Authorization header
    Authorization: Digest username="user", realm="Printer API", nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093", uri="/api/version", response="684d849df474f295771de997e7412ea4"
    """

    # Consume the auth scheme (Digest)
    auto, scheme_spaces = read_until("HorizWhitespace", LabelType.Special)

    # Eat spaces before the parameters
    scheme_spaces.loop('HorizWhitespace', LabelType.Special)

    # We only really read nonce and response, the rest is assumed for
    # performance reasons.
    auth_values = {
        'nonce': auth_value('Nonce'),
        'response': auth_value('Response'),
    }
    tr, terminals, add_unknowns = trie(auth_values)
    tr_start = tr.start()
    line, end = newline()
    tr.join(tr_start, line)

    for parameter in auth_values:
        term = terminals[parameter]
        term.loop("HorizWhitespace", LabelType.Special)
        separator = tr.add_state()
        separator.loop("HorizWhitespace", LabelType.Special)
        term.add_transition('=', LabelType.Char, separator)
        read_par, read_par_end, fallthrough = auth_values[parameter]
        tr.join_transition(separator, read_par, fallthrough=fallthrough)
        read_par_end.add_fallback(tr_start, fallthrough=True)

    # Handling of unknown/ not parsed values
    unknown = tr.add_state()
    for u in add_unknowns:
        u.add_fallback(unknown)
    after_unknown = tr.add_state()
    unknown.add_transition('=', LabelType.Char, after_unknown)
    # For unknown headers without '=' in them
    unknown.add_transition('Whitespace',
                           LabelType.Special,
                           tr_start,
                           fallthrough=True)
    unknown.loop_fallback()
    after_unknown.loop("HorizWhitespace", LabelType.Special)
    ignore_unknown_header, iuh_end, fallthrough = auth_value(None)
    tr.join_transition(after_unknown,
                       ignore_unknown_header,
                       fallthrough=fallthrough)
    iuh_end.add_fallback(tr_start, fallthrough=True)

    auto.join(scheme_spaces, tr)

    return auto, end, True


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


def response(interested):
    """
    Parser for the response, terminating by a body transition.
    """
    line, line_end = resp_line()
    head, body = headers(interested)
    line.join_transition(line_end, head, fallthrough=True)

    return line, body
