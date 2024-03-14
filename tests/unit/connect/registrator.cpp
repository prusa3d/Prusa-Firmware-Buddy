#include "mock_printer.h"
#include "time_mock.h"

#include <connect/registrator.hpp>

#include <catch2/catch.hpp>

using namespace connect_client;
using http::Connection;
using http::Error;
using std::holds_alternative;
using std::min;
using std::monostate;
using std::nullopt;
using std::optional;
using std::string;
using std::variant;

namespace {

class MockConnection final : public Connection {
public:
    string recv_data;
    string send_data;
    MockConnection()
        : Connection(42) {}
    virtual optional<Error> connection(const char *, uint16_t) override {
        return nullopt;
    }
    virtual variant<size_t, Error> rx(uint8_t *buffer, size_t size, bool) override {
        size_t amt = min(recv_data.size(), size);
        memcpy(buffer, recv_data.c_str(), size);
        recv_data.erase(0, amt);
        return amt;
    }
    virtual variant<size_t, Error> tx(const uint8_t *data, size_t size) override {
        send_data += string(reinterpret_cast<const char *>(data), size);
        return size;
    }
    virtual bool poll_readable(uint32_t) override {
        return !recv_data.empty();
    }
};

class ConnFactory final : public RefreshableFactory {
public:
    MockConnection conn;
    virtual const char *host() override {
        return "example.com";
    }
    virtual void invalidate() override {}
    virtual void refresh(const Printer::Config &) override {}
    virtual variant<Connection *, Error> connection() override {
        return &conn;
    }
    virtual bool is_valid() const override {
        return true;
    }
};

class InitPrinter final : public MockPrinter {
public:
    string token;
    InitPrinter(const Params &params)
        : MockPrinter(params) {}
    virtual void init_connect(char *token) override {
        this->token = token;
    }
};

} // namespace

TEST_CASE("Registrator") {
    auto params = params_idle();
    InitPrinter printer(params);
    // Reset the "changed" flag in config
    auto [cfg, changed] = printer.config();
    REQUIRE(changed);

    Registrator registrator(printer);

    advance_time_s(10);

    ConnFactory factory;

    factory.conn.recv_data = "HTTP/1.1 200 OK\r\nCode: abcd\r\n\r\n";
    auto resp = registrator.communicate(factory);
    // Any idea why simple resp == ConnectionStatus::... doesn't work in this case?
    // Also, catch's REQUIRE needs that double (( )) for whatever reasons :-O
    REQUIRE((holds_alternative<ConnectionStatus>(resp) && (get<ConnectionStatus>(resp) == ConnectionStatus::RegistrationCode)));
    // Note:
    // These tests (looking for a substring) are a bit fragile, because they
    // assume none of these fields is split across the chunk boundary. We are
    // simply too lazy to do proper parsing of HTTP & json here, just checking
    // this makes somewhat sense.
    INFO("Sent to connect: " << factory.conn.send_data);
    REQUIRE(factory.conn.send_data.find("\"sn\":\"FAKE-1234\"") != string::npos);
    REQUIRE(factory.conn.send_data.find("\"fingerprint\":\"DEADBEEF\"") != string::npos);
    REQUIRE(strcmp(registrator.get_code(), "abcd") == 0);

    factory.conn.recv_data = "HTTP/1.1 202 Accepted\r\n\r\n";
    factory.conn.send_data = "";

    resp = registrator.communicate(factory);
    // No activity yet, still "cooldown time".
    REQUIRE(holds_alternative<monostate>(resp));
    REQUIRE(factory.conn.recv_data != "");
    REQUIRE(factory.conn.send_data == "");

    advance_time_s(10);
    // But now it does another round of communication.
    // We tell it we don't have it registered yet.
    resp = registrator.communicate(factory);
    REQUIRE(holds_alternative<monostate>(resp));
    REQUIRE(factory.conn.recv_data == "");
    INFO("Sent to connect: " << factory.conn.send_data);
    REQUIRE(factory.conn.send_data != "");
    REQUIRE(factory.conn.send_data.find("Code: abcd\r\n") != string::npos);

    // The user fills in the code to the server and it gets accepted.
    factory.conn.send_data = "";
    factory.conn.recv_data = "HTTP/1.1 200 OK\r\nToken: toktok\r\n\r\n";
    advance_time_s(10);
    resp = registrator.communicate(factory);
    REQUIRE((holds_alternative<ConnectionStatus>(resp) && (get<ConnectionStatus>(resp) == ConnectionStatus::RegistrationDone)));
    REQUIRE(printer.token == "toktok");
}
