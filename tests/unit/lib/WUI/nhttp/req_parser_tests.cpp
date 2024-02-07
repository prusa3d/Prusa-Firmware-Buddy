#include <catch2/catch.hpp>
#include <nhttp/req_parser.h>
#include <nhttp/server.h>
#include <nhttp/handler.h>
#include <http/types.h>

#include <string.h>

using http::MAX_URL_LEN;
using nhttp::Server;
using nhttp::ServerDefs;
using nhttp::handler::RequestParser;
using nhttp::handler::Selector;

class EmptyServerDefs : public ServerDefs {
public:
    virtual const Selector *const *selectors() const override { return {}; }
    virtual const char *get_user_password() const override { return ""; }
    virtual const char *get_apikey() const override { return ""; }
    virtual altcp_pcb *listener_alloc() const override { return {}; }
};

class TestRequestParser : public RequestParser {
public:
    TestRequestParser(Server &server)
        : RequestParser(server) {};

    void set_url(const char *url) {
        url_size = strlen(url);
        strcpy(this->url.data(), url);
    }
};

TEST_CASE("url parser", "[url]") {
    EmptyServerDefs server_defs;
    Server server(server_defs);
    TestRequestParser parser(server);
    char url[MAX_URL_LEN];

    SECTION("small buffer") {
        char small_buffer[1];
        parser.set_url("/test/api/file.gcode");
        REQUIRE_FALSE(parser.uri_filename(small_buffer, sizeof(small_buffer)));
    }

    SECTION("url with encoded and real ?") {
        parser.set_url("/test/api/file%3f.gcode?somemorestuff");
        REQUIRE(parser.uri_filename(url, sizeof(url)));
        REQUIRE(strcmp(url, "/test/api/file?.gcode") == 0);
    }

    SECTION("double dot") {
        parser.set_url("/test/api/../file.gcode");
        REQUIRE_FALSE(parser.uri_filename(url, sizeof(url)));
    }

    SECTION("encoded double dot") {
        parser.set_url("/test/api/%2e%2e/file.gcode");
        REQUIRE_FALSE(parser.uri_filename(url, sizeof(url)));
    }
}
