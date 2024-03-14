from http import connection_header, content_type, read_header_value, response, content_encryption_mode_header

if __name__ == "__main__":
    want_headers = {
        'Content-Length': read_header_value('ContentLength'),
        'Content-Type': content_type(),
        'Command-Id': read_header_value('CommandId'),
        'Code': read_header_value('Code'),
        'Connection': connection_header(),
        'Token': read_header_value('Token'),
        'Content-Encryption-Mode': content_encryption_mode_header(),
    }
    http, final = response(want_headers)
    compiled = http.compile("con::parser::response")
    with open("http_resp_automaton.h", "w") as header:
        header.write(compiled.cpp_header())
    with open("http_resp_automaton.cpp", "w") as file:
        file.write(compiled.cpp_file())
