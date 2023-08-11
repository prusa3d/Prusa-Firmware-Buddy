#include <http/httpc.hpp>
#include <http/resp_parser.h>

#include <catch2/catch.hpp>
#include <cstring>
#include <algorithm>
#include <string>
#include <string_view>

using std::get;
using std::holds_alternative;
using std::min;
using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::variant;
using namespace http;

namespace {

class DummyConnection final : public Connection {
public:
    DummyConnection()
        : Connection(5) {}
    string sent;
    string received;
    virtual optional<Error> connection(const char *, uint16_t) override {
        return nullopt;
    }
    virtual variant<size_t, Error> rx(uint8_t *buffer, size_t len, bool /*nonblock: we never block here*/) override {
        size_t amnt = min(len, received.size());
        memcpy(buffer, received.data(), amnt);
        received.erase(0, amnt);
        return amnt;
    }
    virtual variant<size_t, Error> tx(const uint8_t *buffer, size_t len) override {
        sent += string_view(reinterpret_cast<const char *>(buffer), len);
        return len;
    }
    virtual bool poll_readable(uint32_t /*timeout: no more data will ever arrive*/) override {
        return received.size() > 0;
    }
};

class DummyRequest final : public Request {
private:
    bool done = false;

public:
    virtual const char *url() const override {
        return "/index.html";
    }
    virtual ContentType content_type() const override {
        return ContentType::ApplicationJson;
    }
    virtual Method method() const {
        return Method::Post;
    }
    virtual std::variant<size_t, Error> write_body_chunk(char *data, size_t size) override {
        if (done) {
            return static_cast<size_t>(0);
        } else {
            const char *d = "{\"hello\":\"world\"}";
            const size_t l = strlen(d);
            if (l > size) {
                return Error::Network;
            } else {
                memcpy(data, d, l);
                done = true;
                return l;
            }
        }
    }
};

class ExtractExtra : public ExtraHeader {
public:
    string command_id;
    string code;
    string token;
    virtual void character(char c, HeaderName name) override {
        switch (name) {
        case HeaderName::CommandId:
            command_id += c;
            break;
        case HeaderName::Code:
            code += c;
            break;
        case HeaderName::Token:
            token += c;
            break;
        }
    }
};

class Factory final : public ConnectionFactory {
private:
    Connection *conn;

public:
    Factory(Connection *conn)
        : conn(conn) {}
    virtual std::variant<Connection *, Error> connection() override {
        return conn;
    }
    virtual const char *host() override {
        return "example.com";
    }
    virtual void invalidate() override {}
};

constexpr const char *expected_req = "POST /index.html HTTP/1.1\r\n"
                                     "Host: example.com\r\n"
                                     "Connection: keep-alive\r\n"
                                     "Transfer-Encoding: chunked\r\n"
                                     "Content-Type: application/json\r\n"
                                     "\r\n"
                                     "0011\r\n"
                                     "{\"hello\":\"world\"}\r\n"
                                     "0000\r\n"
                                     "\r\n";

constexpr const char *mock_resp = "HTTP/1.1 204 No Content\r\n"
                                  "Connection: close\r\n"
                                  "\r\n";

constexpr const char *mock_resp_body = "HTTP/1.1 200 OK\r\n"
                                       "Connection: keep-alive\r\n"
                                       "Content-Length: 11\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Command-Id: 42\r\n"
                                       "\r\n"
                                       "Hello world";

constexpr const char *mock_resp_no_connection = "HTTP/1.1 204 No Content\r\n"
                                                "\r\n";

constexpr const char *mock_resp_ancient = "HTTP/1.0 204 No Content\r\n"
                                          "Code: Hello\r\n"
                                          "Token: toktok\r\n"
                                          "\r\n";

Response test_resp_req(const char *server_resp, Status status, ContentType content_type, const char *body, string command_id = "", string code = "", string token = "") {
    DummyConnection conn;
    conn.received = server_resp;
    Factory factory(&conn);

    HttpClient client(factory);

    DummyRequest request;
    ExtractExtra extra;
    auto resp = client.send(request, &extra);
    REQUIRE(holds_alternative<Response>(resp));

    REQUIRE(conn.sent == expected_req);

    auto r = get<Response>(resp);
    REQUIRE(r.status == status);

    size_t exp_len = strlen(body);
    REQUIRE(r.content_length() == exp_len);

    uint8_t buffer[exp_len];
    auto body_resp = r.read_body(buffer, exp_len);
    REQUIRE(holds_alternative<size_t>(body_resp));
    // While the API doesn't officially promise this, we currently don't do short reads.
    REQUIRE(get<size_t>(body_resp) == exp_len);

    uint8_t *b = buffer;
    REQUIRE(string_view(reinterpret_cast<char *>(b), exp_len) == body);

    REQUIRE(r.content_length() == 0);

    REQUIRE(r.content_type == content_type);
    REQUIRE(extra.command_id == command_id);
    REQUIRE(extra.code == code);
    REQUIRE(extra.token == token);

    return r;
}

} // namespace

TEST_CASE("Request - response no content") {
    // Note: content type is on its default octet-stream = "No idea, bunch of bytes I guess"
    auto r = test_resp_req(mock_resp, Status::NoContent, ContentType::ApplicationOctetStream, "");
    REQUIRE_FALSE(r.can_keep_alive);
}

TEST_CASE("Request - response with body") {
    auto r = test_resp_req(mock_resp_body, Status::Ok, ContentType::ApplicationJson, "Hello world", "42");
    REQUIRE(r.can_keep_alive);
}

TEST_CASE("Request - no connection header") {
    // Note: content type is on its default octet-stream = "No idea, bunch of bytes I guess"
    auto r = test_resp_req(mock_resp_no_connection, Status::NoContent, ContentType::ApplicationOctetStream, "");
    REQUIRE(r.can_keep_alive); // Implicit by HTTP/1.1
}
TEST_CASE("Request - ancient") {
    // Note: content type is on its default octet-stream = "No idea, bunch of bytes I guess"
    auto r = test_resp_req(mock_resp_ancient, Status::NoContent, ContentType::ApplicationOctetStream, "", "", "Hello", "toktok");
    REQUIRE_FALSE(r.can_keep_alive); // Implicit by HTTP/1.0
}
