/*
 * Tests for the HTTP server layer.
 *
 * These are mostly meant as tests for the "server" layer, that it all
 * works together. For example the tets for the request parser are
 * done in the automata tests (and we probably should have some for
 * the ReqParser handler too).
 *
 * This unfortunately calls for quite a lot of mocking, both on the
 * networking side and on the application/content side.
 *
 * It also brings in a lot of/most of the code _around_ the server; we
 * may want to find some way to separate them better.
 */

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <nhttp/server.h>
#include <nhttp/common_selectors.h>
#include <nhttp/headers.h>
#include <lwip/altcp.h>
#include <lwip/priv/altcp_priv.h>

#include <catch2/catch.hpp>
#include <memory>
#include <vector>

using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using namespace nhttp;
using namespace nhttp::handler;
using namespace nhttp::handler::selectors;

namespace {

class MockConn;

struct ConnInfo {
    bool alive = true;
    bool listening = false;
    string sent_data;
    MockConn *conn;
};

class MockConn : public altcp_pcb {
public:
    std::shared_ptr<ConnInfo> info;

private:
    static void set_poll(altcp_pcb *me, uint8_t interval) {
    }
    static void recved(altcp_pcb *me, uint16_t len) {
    }
    static err_t bind(altcp_pcb *me, const ip_addr_t *ipaddr, uint16_t port) {
        return ERR_OK;
    }
    static err_t connect(altcp_pcb *me, const ip_addr_t *ipaddr, uint16_t port, altcp_connected_fn connected) {
        connected(me->arg, me, ERR_OK);
        return ERR_OK;
    }
    static altcp_pcb *listen(altcp_pcb *me, uint8_t backlog, err_t *err) {
        static_cast<MockConn *>(me)->info->listening = true;
        return me;
    }
    static void abort(altcp_pcb *me) {
        static_cast<MockConn *>(me)->info->alive = false;
    }
    static err_t close(altcp_pcb *me) {
        static_cast<MockConn *>(me)->info->alive = false;
        return ERR_OK;
    }

    static err_t shutdown(altcp_pcb *me, int shut_rx, int shut_tx) {
        static_cast<MockConn *>(me)->info->alive = false;
        return ERR_OK;
    }
    static err_t write(altcp_pcb *me, const void *dataptr, uint16_t len, uint8_t apiflags) {
        MockConn *me_typed = static_cast<MockConn *>(me);
        REQUIRE(me_typed->info->alive);
        me_typed->info->sent_data += string(static_cast<const char *>(dataptr), len);
        return ERR_OK;
    }

    static err_t output(altcp_pcb *me) {
        return ERR_OK;
    }

    static uint16_t mss(altcp_pcb *me) {
        return TCP_MSS;
    }

    static uint16_t sndbuf(altcp_pcb *me) {
        return TCP_MSS;
    }

    static uint16_t sndqueuelen(altcp_pcb *me) {
        return 0;
    }

    static void nagle_disable(altcp_pcb *me) {}

    static void nagle_enable(altcp_pcb *me) {}

    static int nagle_disabled(altcp_pcb *me) {
        return 0;
    }

    static void setprio(altcp_pcb *me, uint8_t prio) {}

    static void dealloc(altcp_pcb *me) {
        MockConn *me_typed = static_cast<MockConn *>(me);
        me_typed->info->conn = NULL;
        delete me_typed;
    }

    static err_t get_tcp_addrinfo(altcp_pcb *me, int local, ip_addr_t *addr, uint16_t *port) {
        return ERR_VAL;
    }

    static ip_addr_t *get_ip(altcp_pcb *me, int local) {
        return NULL;
    }

    static uint16_t get_port(altcp_pcb *me, int local) {
        return 0;
    }

    static const constexpr altcp_functions funcs = {
        &set_poll,
        &recved,
        &bind,
        &connect,
        &listen,
        &abort,
        &close,
        &shutdown,
        &write,
        &output,
        &mss,
        &sndbuf,
        &sndqueuelen,
        &nagle_disable,
        &nagle_enable,
        &nagle_disabled,
        &setprio,
        &dealloc,
        &get_tcp_addrinfo,
        &get_ip,
        &get_port
    };

public:
    MockConn()
        : info(new ConnInfo) {
        info->conn = this;
        std::memset(static_cast<altcp_pcb *>(this), 0, sizeof(altcp_pcb));
        fns = &funcs;
    }
};

// Sends a fake index page, for testing purposes.
class FakeIndex final : public Selector {
public:
    virtual optional<ConnectionState> accept(const RequestParser &parser) const override {
        char filename[MAX_URL_LEN + 1];

        if (!parser.uri_filename(filename, sizeof(filename))) {
            return nullopt;
        }

        if (strcmp(filename, "/") == 0) {
            strncpy(filename, "/index.html", MAX_URL_LEN);
        }

        if (strcmp(filename, "/index.html") == 0) {
            static const char *extra_hdrs[] = { "Fake: YesOfCourse\r\n", nullptr };
            return SendStaticMemory("<h1>Hello world</h1>", guess_content_by_ext(filename), parser.can_keep_alive(), extra_hdrs);
        }

        return nullopt;
    }
};

const FakeIndex fake_index;

class FakeApi final : public Selector {
public:
    virtual optional<ConnectionState> accept(const RequestParser &parser) const override {
        char filename[MAX_URL_LEN + 1];

        if (!parser.uri_filename(filename, sizeof(filename))) {
            return nullopt;
        }

        if (strcmp(filename, "/secret.html") == 0) {
            if (parser.authenticated()) {
                static const char *extra_hdrs[] = { "Fake: YesOfCourse\r\n", nullptr };
                return SendStaticMemory("<html><body><h1>Don't tell anyone!</h1>", guess_content_by_ext(filename), parser.can_keep_alive(), extra_hdrs);
            } else {
                return StatusPage(Status::Unauthorized, parser.status_page_handling(), false);
            }
        }

        return nullopt;
    }
};

const FakeApi fake_api;

class MockServerDefs final : public ServerDefs {
private:
    friend class MockServer;
    vector<shared_ptr<ConnInfo>> &infos;
    static const constexpr Selector *selectors_array[] = { &validate_request, &fake_index, &fake_api, &unknown_request };
    altcp_pcb *new_conn() const {
        MockConn *conn = new MockConn;
        infos.push_back(conn->info);
        return conn;
    }
    string api_key;

public:
    MockServerDefs(vector<shared_ptr<ConnInfo>> &conn_infos)
        : infos(conn_infos) {}
    virtual const Selector *const *selectors() const override { return selectors_array; }
    virtual const char *get_api_key() const override { return api_key.c_str(); }
    virtual altcp_pcb *listener_alloc() const override {
        auto conn = new_conn();
        return altcp_listen(conn);
    }
};

class MockServer {
private:
    vector<shared_ptr<ConnInfo>> infos;
    MockServerDefs server_defs;
    Server server;

public:
    MockServer()
        : server_defs(infos)
        , server(server_defs) {
        server.start();
    }

    /*
     * Initialize a new connection to the server and put it into the handlers->infos.
     *
     * Returns the index of the connection.
     */
    size_t new_conn() {
        // We assume the 0th connection is the server's listening one.
        auto conn = server_defs.new_conn();
        const size_t idx = server_defs.infos.size() - 1;
        auto listener = server_defs.infos[0];
        REQUIRE(listener->listening);
        REQUIRE(listener->conn != nullptr);
        auto listener_conn = listener->conn;
        listener_conn->accept(listener_conn->arg, conn, ERR_OK);
        return idx;
    }

    void send(size_t conn_idx, const string &data) {
        REQUIRE(conn_idx < infos.size());
        auto &info = infos[conn_idx];
        REQUIRE(info->alive);
        auto conn = info->conn;
        REQUIRE(conn != nullptr);
        // This is a bit of an abuse. We set the ref count to "many" and that
        // makes sure it doesn't get freed. This one doesn't own the data.
        //
        // Strictly speaking, this is incorrect as the recipient may decide to
        // hold onto the buffer after we are done here (by incrementing the ref
        // count). For this test know it currently doesn't happen.
        pbuf buffer;
        memset(&buffer, 0, sizeof buffer);
        // Un-constify the thing...
        unique_ptr<char[]> nonconst_data(new char[data.size() + 1]);
        strcpy(nonconst_data.get(), data.c_str());
        buffer.payload = nonconst_data.get();
        buffer.len = data.size();
        buffer.tot_len = data.size();
        buffer.ref = 42;
        conn->recv(conn->arg, conn, &buffer, ERR_OK);
        REQUIRE(buffer.ref > 0);
    }

    // Read as much as the server is willing to produce now.
    string recv_all(size_t conn_idx) {
        REQUIRE(conn_idx < infos.size());
        auto &info = infos[conn_idx];
        // Note: we do _not_ check alive here - it's OK for the connection to
        // be already closed.
        auto conn = info->conn;
        string result;
        while (!info->sent_data.empty()) {
            const size_t buff_len = info->sent_data.size();
            result += info->sent_data;
            info->sent_data.clear();
            if (conn != nullptr && info->alive) {
                conn->sent(conn->arg, conn, buff_len);
            }
        }

        return result;
    }

    void set_api_key(string key) {
        server_defs.api_key = std::move(key);
    }
};

const constexpr char *GET_INDEX = "GET / HTTP/1.1\r\n\r\n";

void check_index(const string &response) {
    INFO("Have response: " + response);
    // Optimally, we would actually parse the response. But for now the tests
    // only peek into it if it looks more or less OK and we do it in dirty way.
    REQUIRE(response.find("HTTP/1.1 200 OK") == 0);
    REQUIRE(response.find("Content-Type: text/html") != string::npos);
    // Body separator + our fake body
    REQUIRE(response.find("\r\n\r\n<h1>Hello world</h1>") != string::npos);
}

void check_unauth(const string &response) {
    INFO("Response: " + response);
    REQUIRE(response.find("HTTP/1.1 401 Unauthorized\r\n") == 0);
    REQUIRE(response.find("Content-Type: text/plain") != string::npos);
    REQUIRE(response.find("WWW-Authenticate: ApiKey") != string::npos);
    REQUIRE(response.find("\r\n\r\n401: Unauthorized") != string::npos);
}

}

TEST_CASE("Get index") {
    MockServer server;
    const size_t client_conn = server.new_conn();
    REQUIRE(client_conn == 1);
    server.send(client_conn, GET_INDEX);
    const auto response = server.recv_all(client_conn);
    check_index(response);
}

TEST_CASE("Multiple conns") {
    MockServer server;
    const size_t cl1 = server.new_conn();
    const size_t cl2 = server.new_conn();
    const size_t cl3 = server.new_conn();

    REQUIRE(server.recv_all(cl1).empty());
    REQUIRE(server.recv_all(cl2).empty());
    REQUIRE(server.recv_all(cl3).empty());

    server.send(cl2, GET_INDEX);
    REQUIRE(server.recv_all(cl1).empty());
    REQUIRE(server.recv_all(cl3).empty());

    check_index(server.recv_all(cl2));

    server.send(cl1, GET_INDEX);
    check_index(server.recv_all(cl1));
}

TEST_CASE("Not found") {
    MockServer server;
    const size_t client_conn = server.new_conn();
    REQUIRE(client_conn == 1);
    server.send(client_conn, "GET /not-here HTTP/1.1\r\n\r\n");
    const auto response = server.recv_all(client_conn);
    INFO("Response: " + response);
    REQUIRE(response.find("HTTP/1.1 404 Not Found\r\n") == 0);
    REQUIRE(response.find("Content-Type: text/plain") != string::npos);
    REQUIRE(response.find("\r\n\r\n404: Not Found") != string::npos);
}

TEST_CASE("Authenticated") {
    MockServer server;
    server.set_api_key("SECRET");

    const size_t client_conn = server.new_conn();
    REQUIRE(client_conn == 1);
    server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: SECRET\r\n\r\n");
    const auto response = server.recv_all(client_conn);
    INFO("Response: " + response);
    REQUIRE(response.find("HTTP/1.1 200 OK\r\n") == 0);
    REQUIRE(response.find("Content-Type: text/html") != string::npos);
    REQUIRE(response.find("\r\n\r\n<html>") != string::npos);
}

TEST_CASE("Not authenticated") {
    MockServer server;
    server.set_api_key("SECRET");

    const size_t client_conn = server.new_conn();
    REQUIRE(client_conn == 1);

    SECTION("No key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\n\r\n");
    }

    SECTION("Empty key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: \r\n\r\n");
    }

    SECTION("Wrong key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: Password!\r\n\r\n");
    }

    SECTION("Wrong case") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: Secret\r\n\r\n");
    }

    SECTION("Extra") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: SECRET.\r\n\r\n");
    }

    SECTION("Missing") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: SECRE\r\n\r\n");
    }

    const auto response = server.recv_all(client_conn);
    check_unauth(response);
}

// If the server doesn't have an API key set, no combination of
// missing key, empty key, etc, will let us in (actually, nothing will
// let us in).
TEST_CASE("No Api Key configured") {
    MockServer server;

    const size_t client_conn = server.new_conn();
    REQUIRE(client_conn == 1);

    SECTION("No key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\n\r\n");
    }

    SECTION("Empty key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: \r\n\r\n");
    }

    SECTION("Wrong key") {
        server.send(client_conn, "GET /secret.html HTTP/1.1\r\nX-Api-Key: Password!\r\n\r\n");
    }

    const auto response = server.recv_all(client_conn);
    check_unauth(response);
}

/*
 * TODO: Further test ideas (non-exhaustive)
 *
 * * Reading by parts (slow-reading client).
 * * Reading by parts with multiple connections.
 * * Mangled requests (not HTTP).
 * * Very Big Request.
 * * Request sent by multiple packets.
 * * Multiple instances of the same header.
 * * Keep-alive connections.
 * * Custom generated responses.
 *
 * * Posts… these are kind of chapter of its own.
 */
