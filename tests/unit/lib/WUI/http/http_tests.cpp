/*
 * Few tests for the http server.
 *
 * While the http server is 3rd-party code, it is:
 * * exposed to the outer world, parts of it without authentication.
 * * modified by us (what exact modifications happened is unclear, as the big
 *   bulk comes in one huge commit that clearly contains already modified code).
 * * scary. Seriously scary.
 * * doesn't have upstream tests we could borrow.
 *
 * Therefore, we opt to have our own to cover it a bit.
 *
 * As we would like to replace the HTTP code eventually, the tests are done as
 * fast as reasonable ‒ the code here contains certain ugly shortcuts, stubs
 * out a lot of things, etc. The future version might not need these things (as
 * it probably won't be tied deeply into LwIP).
 *
 * As more tests are added, the stubs may be turned into mocks.
 *
 * Also, as we plan to have fuzzing for the server, we may want to extract the
 * mocks/stubs into a common file used by both the tests and the fuzzing.
 */
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <http/httpd.h>
#include <lwip/altcp.h>
#include <lwip/priv/altcp_priv.h>

#include <catch2/catch.hpp>
#include <memory>
#include <vector>

using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

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

class MockHandler : public HttpHandlers {
public:
    vector<shared_ptr<ConnInfo>> infos;

private:
    friend class MockServer;
    altcp_pcb *mock_alloc_inner() {
        MockConn *conn = new MockConn;
        infos.push_back(conn->info);
        return conn;
    }
    static altcp_pcb *mock_alloc(void *arg, uint8_t ip_type) {
        return static_cast<MockHandler *>(arg)->mock_alloc_inner();
    }

public:
    MockHandler() {
        listener_alloc.alloc = mock_alloc;
        listener_alloc.arg = this;
    }
};

class MockServer {
private:
    unique_ptr<MockHandler> handlers;
    unique_ptr<struct altcp_pcb, decltype(&httpd_free)> server;

public:
    MockServer()
        : handlers(new MockHandler())
        , server(httpd_new(handlers.get(), 666), httpd_free) {
    }

    /*
     * Initialize a new connection to the server and put it into the handlers->infos.
     *
     * Returns the index of the connection.
     */
    size_t new_conn() {
        // We assume the 0th connection is the server's listening one.
        auto conn = handlers->mock_alloc_inner();
        const size_t idx = handlers->infos.size() - 1;
        auto listener = handlers->infos[0];
        REQUIRE(listener->listening);
        REQUIRE(listener->conn != nullptr);
        auto listener_conn = listener->conn;
        listener_conn->accept(listener_conn->arg, conn, ERR_OK);
        return idx;
    }

    void send(size_t conn_idx, const string &data) {
        REQUIRE(conn_idx < handlers->infos.size());
        auto &info = handlers->infos[conn_idx];
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
        REQUIRE(conn_idx < handlers->infos.size());
        auto &info = handlers->infos[conn_idx];
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
};

const constexpr char *GET_INDEX = "GET / HTTP/1.1r\r\n\r\n";

void check_index(const string &response) {
    // Optimally, we would actually parse the response. But for now the tests
    // only peek into it if it looks more or less OK and we do it in dirty way.
    REQUIRE(response.find("HTTP/1.0 200 OK") == 0);
    REQUIRE(response.find("Content-Type: text/html") != string::npos);
    // Body separator + gzip magic number
    REQUIRE(response.find("\r\n\r\n\x1F\x8B") != string::npos);
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

/*
 * Connect more than one client and send requests on them out of order.
 */
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
 * * Authentication (positive and negative).
 * * Custom generated responses.
 * * Not-found.
 *
 * * Posts… these are kind of chapter of its own.
 */
