#include <until_comma.h>
#include <http.h>
#include <http_method.h>
#include <http_req.h>
#include "test_execution.h"

#include <catch2/catch.hpp>
#include <string_view>

using namespace automata;
using std::get;
using std::string;
using std::string_view;

namespace {

/*
 * An automaton that accepts everything until a comma.
 */
const Automaton until_comma(test::until_comma::paths, test::until_comma::transitions, test::until_comma::states);
/**
 * Selection of HTTP method.
 */
const Automaton method(test::http_method::paths, test::http_method::transitions, test::http_method::states);

const Automaton req_line(test::http_req::paths, test::http_req::transitions, test::http_req::states);
const Automaton http_request(test::http::paths, test::http::transitions, test::http::states);

} // namespace

TEST_CASE("Until comma") {
    TestExecution ex(until_comma);

    REQUIRE(get<0>(ex.consume("Hello world")) == ExecutionControl::Continue);
    REQUIRE(ex.events.empty());
    REQUIRE(get<0>(ex.consume("xp,")) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    REQUIRE(ex.events[0].leaving_state == 0);
    REQUIRE(ex.events[0].entering_state == test::until_comma::Names::Comma);
    REQUIRE(ex.events[0].payload == ',');
}

TEST_CASE("Method") {
    using test::http_method::Names;
    TestExecution ex(method);

    std::string_view text;
    Names expected;
    SECTION("Get") {
        text = "GET ";
        expected = Names::MethodGet;
    }

    SECTION("Post") {
        text = "POST ";
        expected = Names::MethodPost;
    }

    SECTION("Put") {
        text = "PUT ";
        expected = Names::MethodPut;
    }

    REQUIRE(get<0>(ex.consume(text)) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    REQUIRE(ex.events[0].leaving_state == expected);
}

TEST_CASE("Method no-case") {
    TestExecution ex(method);

    REQUIRE(get<0>(ex.consume("get ")) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    REQUIRE(ex.events[0].leaving_state == test::http_method::Names::MethodGet);
}

TEST_CASE("Unknown method") {
    TestExecution ex(method);

    REQUIRE(get<0>(ex.consume("XIMADETHISUP ")) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() == 1);
    REQUIRE(ex.events[0].leaving_state == test::http_method::Names::MethodUnknown);
}

TEST_CASE("Request line") {
    using test::http_req::Names;
    TestExecution ex(req_line);

    REQUIRE(get<0>(ex.consume("GET /index.html HTTP/1.1")) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() > 3);
    REQUIRE(ex.events[0].leaving_state == Names::MethodGet);
    REQUIRE(ex.collect_entered(Names::Url) == "/index.html");
    REQUIRE(ex.collect_entered(Names::Version) == "1.1");
}

TEST_CASE("Request fuzzy") {
    using test::http_req::Names;
    TestExecution ex(req_line);

    REQUIRE(get<0>(ex.consume("GeT \t/index.html    HttP/1.15\n")) == ExecutionControl::Continue);
    REQUIRE(ex.events.size() > 3);
    REQUIRE(ex.events[0].leaving_state == Names::MethodGet);
    REQUIRE(ex.collect_entered(Names::Url) == "/index.html");
    REQUIRE(ex.collect_entered(Names::Version) == "1.15");
}

const string_view request = string_view("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nX-Api-Key: hello\r\nContent-Length: 4\r\n\r\nBody");

TEST_CASE("Request line only") {
    TestExecution ex(req_line);

    const auto [result, consumed] = ex.consume(request);
    REQUIRE(result == ExecutionControl::NoTransition);
    REQUIRE(consumed == 16);
    REQUIRE(request.substr(consumed).find("Host:") == 0);
}

TEST_CASE("Whole request") {
    using test::http::Names;
    TestExecution ex(http_request);

    const auto [result, consumed] = ex.consume(request);
    REQUIRE(result == ExecutionControl::NoTransition);

    REQUIRE(ex.events[0].leaving_state == Names::MethodGet);
    REQUIRE(ex.collect_entered(Names::Url) == "/");
    REQUIRE(ex.collect_entered(Names::Version) == "1.1");
    REQUIRE(ex.collect_entered(Names::ContentLength) == "4");
    REQUIRE(ex.collect_entered(Names::XApiKey) == "hello");
    REQUIRE(ex.events.back().entering_state == Names::Body);

    REQUIRE(request.substr(consumed) == "Body");
}

TEST_CASE("Whole request - fuzzy") {
    using test::http::Names;
    TestExecution ex(http_request);
    const string_view request = string_view("PoSt /index.HTML \t httP/0.14\nComment: Multi\n  line\n header\rX-Api-Key: hello\n world\rContent-Length: 4\n\nBody");

    const auto [result, consumed] = ex.consume(request);
    REQUIRE(result == ExecutionControl::NoTransition);

    REQUIRE(ex.events[0].leaving_state == Names::MethodPost);
    REQUIRE(ex.collect_entered(Names::Url) == "/index.HTML");
    REQUIRE(ex.collect_entered(Names::Version) == "0.14");
    REQUIRE(ex.collect_entered(Names::ContentLength) == "4");
    REQUIRE(ex.collect_entered(Names::XApiKey) == "helloworld");
    REQUIRE(ex.events.back().entering_state == Names::Body);

    REQUIRE(request.substr(consumed) == "Body");
}

TEST_CASE("X-Api-Key first") {
    using test::http::Names;
    TestExecution ex(http_request);
    const string_view request = string_view("GET /api/version HTTP/1.1\r\nX-Api-Key: 12345678\r\nAccept: */*\r\nHost: 1.2.3.4\r\n\r\n");
    const auto [result, consumed] = ex.consume(request);

    REQUIRE(result == ExecutionControl::Continue);
    REQUIRE(ex.events[0].leaving_state == Names::MethodGet);
    REQUIRE(ex.collect_entered(Names::Url) == "/api/version");
    REQUIRE(ex.collect_entered(Names::Version) == "1.1");
    REQUIRE(ex.collect_entered(Names::XApiKey) == "12345678");
    REQUIRE(ex.events.back().entering_state == Names::Body);
}

TEST_CASE("Extract boundary") {
    using test::http::Names;
    const char *boundary = nullptr;

    SECTION("Common") {
        boundary = "mutlipart/mixed; boundary=hello";
    }

    SECTION("Extra before") {
        boundary = "mutlipart/mixed; charset=utf8; boundary=hello";
    }

    SECTION("Extra after") {
        boundary = "mutlipart/mixed; boundary=hello; whatever";
    }

    SECTION("Nospaces") {
        boundary = "mutlipart/mixed; boundary=hello";
    }

    SECTION(" Extra  spaces ") {
        boundary = "mutlipart/mixed ; boundary=hello ";
    }

    SECTION("Split before colon") {
        boundary = "mutlipart/mixed\r\n\t; boundary=hello";
    }

    SECTION("Split after colon") {
        boundary = "mutlipart/mixed;\r\n\t boundary=hello";
    }

    const string request = string("GET / HTTP/1.1\r\nContent-Type: ") + boundary + "\r\n" + "Content-Length: 42" + "\r\n\r\n";

    TestExecution ex(http_request);
    ex.consume(request);
    REQUIRE(ex.collect_entered(Names::ContentLength) == "42");
    REQUIRE(ex.collect_entered(Names::Boundary) == "hello");
}

TEST_CASE("Connection close") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nConnection: close\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::ConnectionClose));
    REQUIRE_FALSE(ex.contains_enter(Names::ConnectionKeepAlive));
}

TEST_CASE("Connection keep alive") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nConnection: Keep-Alive\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::ConnectionKeepAlive));
    REQUIRE_FALSE(ex.contains_enter(Names::ConnectionClose));
}

TEST_CASE("Accept json") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nAccept: application/json\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::AcceptJson));
    REQUIRE_FALSE(ex.contains_enter(Names::ConnectionClose));
}

TEST_CASE("Encryption Mode CTR") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nContent-Encryption-Mode: AES-CTR\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::ContentEncryptionModeCTR));
    REQUIRE_FALSE(ex.contains_enter(Names::ContentEncryptionModeCBC));
}

TEST_CASE("Encryption Mode CBC") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nContent-Encryption-Mode: AES-CBC\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::ContentEncryptionModeCBC));
    REQUIRE_FALSE(ex.contains_enter(Names::ContentEncryptionModeCTR));
}

TEST_CASE("No connection") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\n\r\n");
    REQUIRE_FALSE(ex.contains_enter(Names::ConnectionKeepAlive));
    REQUIRE_FALSE(ex.contains_enter(Names::ConnectionClose));
}

TEST_CASE("Print after upload") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nPrint-After-Upload: true\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::PrintAfterUpload));
}

TEST_CASE("Print after upload CaSe") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nPrint-After-Upload: TruE\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::PrintAfterUpload));
}

TEST_CASE("Print after upload numeric") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nPrint-After-Upload: 1\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::PrintAfterUploadNumeric));
}

TEST_CASE("No print after upload") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET / HTTP/1.1\r\nPrint-After-Upload: false\r\n\r\n");
    REQUIRE_FALSE(ex.contains_enter(Names::PrintAfterUpload));
    REQUIRE_FALSE(ex.contains_enter(Names::PrintAfterUploadNumeric));
}

TEST_CASE("Digest auth whole") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET /api/version HTTP/1.1\r\nAuthorization: Digest username=\"user\", realm=\"Printer API\", nonce=\"dcd98b7102dd2f0e\", uri=\"/api/version\", response=\"684d849df474f295771de997e7412ea4\"\r\n\r\n");
    REQUIRE(ex.collect_entered(Names::Nonce) == "dcd98b7102dd2f0e");
    REQUIRE(ex.collect_entered(Names::Response) == "684d849df474f295771de997e7412ea4");
}

TEST_CASE("Digest auth with algorithm") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET /api/version HTTP/1.1\r\nAuthorization: Digest username=\"user\", realm=\"Printer API\", nonce=\"dcd98b7102dd2f0e\", uri=\"/api/version\", algorithm=MD5, response=\"684d849df474f295771de997e7412ea4\"\r\n\r\n");
    REQUIRE(ex.collect_entered(Names::Nonce) == "dcd98b7102dd2f0e");
    REQUIRE(ex.collect_entered(Names::Response) == "684d849df474f295771de997e7412ea4");
}

TEST_CASE("Digest auth without quotes") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET /api/version HTTP/1.1\r\nAuthorization: Digest username=\"user\", realm=\"Printer API\", nonce=dcd98b7102dd2f0e, uri=\"/api/version\", response=684d849df474f295771de997e7412ea4\r\n\r\n");
    REQUIRE(ex.collect_entered(Names::NonceUnquoted) == "dcd98b7102dd2f0e");
    REQUIRE(ex.collect_entered(Names::ResponseUnquoted) == "684d849df474f295771de997e7412ea4");
    REQUIRE(ex.contains_enter(Names::Body));
}

TEST_CASE("Basic auth mistake") {
    using test::http::Names;
    TestExecution ex(http_request);
    ex.consume("GET /api/v1/info HTTP/1.1\r\nAuthorization: Basic bWFrZXI6bm90X215X3B3\r\nUser-Agent: curl/8.8.0\r\nAccept: */*\r\n\r\n");
    REQUIRE(ex.contains_enter(Names::Body));
}
