from http import response

if __name__ == "__main__":
    want_headers = {}
    http, final = response(want_headers)
    compiled = http.compile("con::parser::response")
    with open("http_resp_automaton.h", "w") as header:
        header.write(compiled.cpp_header())
    with open("http_resp_automaton.cpp", "w") as file:
        file.write(compiled.cpp_file())
