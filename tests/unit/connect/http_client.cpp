#include <httpc.hpp>

#include <catch2/catch.hpp>
#include <cstring>
#include <algorithm>
#include <string>
#include <string_view>

using http::Status;
using std::get;
using std::holds_alternative;
using std::min;
using std::nullopt;
using std::optional;
using std::string;
using std::string_view;
using std::variant;
using namespace con;

namespace {

class DummyConnection final : public Connection {
public:
    string sent;
    string received;
    virtual optional<Error> connection(const char *, uint16_t) override {
        return nullopt;
    }
    virtual variant<size_t, Error> rx(uint8_t *buffer, size_t len) override {
        size_t amnt = min(len, received.size());
        memcpy(buffer, received.data(), amnt);
        received.erase(0, amnt);
        return amnt;
    }
    virtual variant<size_t, Error> tx(const uint8_t *buffer, size_t len) override {
        sent += string_view(reinterpret_cast<const char *>(buffer), len);
        return len;
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
    virtual const HeaderOut *extra_headers() const override {
        return nullptr;
    }
    virtual std::variant<size_t, Error> write_body_chunk(char *data, size_t size) override {
        if (done) {
            return static_cast<size_t>(0);
        } else {
            const char *d = "{\"hello\":\"world\"}";
            const size_t l = strlen(d);
            if (l > size) {
                return Error::WRITE_ERROR;
            } else {
                memcpy(data, d, l);
                done = true;
                return l;
            }
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
                                     "Connection: close\r\n"
                                     "Transfer-Encoding: chunked\r\n"
                                     "Content-Type: application/json\r\n"
                                     "\r\n"
                                     "0011\r\n"
                                     "{\"hello\":\"world\"}\r\n"
                                     "0000\r\n"
                                     "\r\n";

constexpr const char *mock_resp = "204 No Content\r\n"
                                  "Connection: close\r\n"
                                  "\r\n";

}

TEST_CASE("Request - response no content") {
    DummyConnection conn;
    conn.received = mock_resp;
    Factory factory(&conn);

    HttpClient client(factory);

    DummyRequest request;
    auto resp = client.send(request);
    REQUIRE(holds_alternative<Response>(resp));

    REQUIRE(conn.sent == expected_req);

    auto r = get<Response>(resp);
    REQUIRE(r.status == Status::NoContent);
}
