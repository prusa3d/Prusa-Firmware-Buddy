"""
Testing automata.

This script generates few automata for checking the generator works as
expected. They are generated and used during the unit tests.
"""
from common import Automaton, LabelType
from http import accept_header, connection_header, methods, read_boundary, read_header_value, req_line, request, print_after_upload_header, authorization_header, content_encryption_mode_header


def output(name, automaton):
    compiled = automaton.compile("automata::test::" + name)
    with open(name + ".h", "w") as header:
        header.write(compiled.cpp_header())
    with open(name + ".cpp", "w") as file:
        file.write(compiled.cpp_file())


# Automaton no. 1: accept everything up to a comma, eg `.*,`.
until_comma = Automaton()
start = until_comma.start()
comma = until_comma.add_state("Comma")
comma.mark_enter()
start.add_transition(',', LabelType.Char, comma)
start.add_transition("All", LabelType.Special, start)

output("until_comma", until_comma)

# Automaton no. 2: http method terminated by a space
method, final = methods()
output("http_method", method)

# Automaton no. 3: HTTP request line
http_req, final = req_line()
output("http_req", http_req)

# A whole request line + headers, ending with a body
want_headers = {
    'X-Api-Key': read_header_value('XApiKey'),
    'Authorization': authorization_header(),
    'Content-Length': read_header_value('ContentLength'),
    'Content-Type': read_boundary(),
    'Print-After-Upload': print_after_upload_header(),
    'Connection': connection_header(),
    'Accept': accept_header(),
    'Content-Encryption-Mode': content_encryption_mode_header(),
}
http, final = request(want_headers)
output("http", http)
