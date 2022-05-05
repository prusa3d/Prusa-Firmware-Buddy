#include "connect.hpp"
#include "httpc.hpp"
#include "httpc_data.hpp"
#include "tls/tls.hpp"

#include <cassert>
#include <debug.h>
#include <os_porting.hpp>
#include <cstring>
#include <optional>
#include <variant>
#include <socket.hpp>
#include <cmsis_os.h>

#include <log.h>

using http::Status;
using std::get;
using std::holds_alternative;
using std::nullopt;
using std::optional;
using std::variant;

LOG_COMPONENT_DEF(connect, LOG_SEVERITY_DEBUG);

namespace con {

namespace {

    class PreparedFactory final : public ConnectionFactory {
    private:
        const char *hostname;
        uint16_t port;
        Connection *conn;

    public:
        PreparedFactory(const char *hostname, uint16_t port, Connection *conn)
            : hostname(hostname)
            , port(port)
            , conn(conn) {}
        virtual std::variant<Connection *, Error> connection() override {
            if (auto err = conn->connection(hostname, port); err.has_value()) {
                return *err;
            }
            return conn;
        }
        virtual const char *host() override {
            return hostname;
        }
        virtual void invalidate() override {
            // NOP, this thing is single-use anyway.
        }
    };

    class BasicRequest final : public Request {
    private:
        core_interface &core;
        const printer_info_t &info;
        RequestType req_type;
        HeaderOut hdrs[3];
        bool done = false;

    public:
        BasicRequest(core_interface &core, const printer_info_t &info, const configuration_t &config, RequestType req_type)
            : core(core)
            , info(info)
            , req_type(req_type)
            , hdrs {
                { "Fingerprint", info.fingerprint },
                { "Token", config.token },
                { nullptr, nullptr }
            } {}
        virtual const char *url() const override {
            switch (req_type) {
            case RequestType::Telemetry:
                return "/p/telemetry";
            case RequestType::SendInfo:
                return "/p/events";
            default:
                assert(0);
                return "";
            }
        }
        virtual ContentType content_type() const override {
            return ContentType::ApplicationJson;
        }
        virtual Method method() const override {
            return Method::Post;
        }
        virtual const HeaderOut *extra_headers() const override {
            return hdrs;
        }
        virtual variant<size_t, Error> write_body_chunk(char *data, size_t size) override {
            if (done) {
                return 0U;
            } else {
                done = true;
                switch (req_type) {
                case RequestType::Telemetry: {
                    device_params_t params = core.get_data();
                    httpc_data renderer;
                    return renderer.telemetry(params, data, size);
                }
                case RequestType::SendInfo: {
                    httpc_data renderer;
                    return renderer.info(info, data, size, 0);
                }
                default: {
                    assert(0);
                    return Error::INVALID_PARAMETER_ERROR;
                }
                }
            }
        }
    };

}

optional<Error> connect::handle_server_resp(Response resp) {
    // TODO: Not implemented
    return nullopt;
}

optional<Error> connect::communicate() {
    configuration_t config = core.get_connect_config();

    if (!config.enabled) {
        return nullopt;
    }

    // TODO: Any nicer way to do this in C++?
    variant<tls, socket_con> connection_storage;
    Connection *connection;
    if (config.tls) {
        connection_storage.emplace<tls>();
        connection = &std::get<tls>(connection_storage);
    } else {
        connection_storage.emplace<socket_con>();
        connection = &std::get<socket_con>(connection_storage);
    }

    PreparedFactory conn_factory(config.host, config.port, connection);
    HttpClient http(conn_factory);

    BasicRequest request(core, printer_info, config, next_request);
    const auto result = http.send(request);

    if (holds_alternative<Error>(result)) {
        conn_factory.invalidate();
        next_request = RequestType::SendInfo;
        return get<Error>(result);
    } else {
        // TODO: Handle the response. Once we have some.
        next_request = RequestType::Telemetry;
    }

    Response resp = get<Response>(result);
    switch (resp.status) {
    // The server has nothing to tell us
    case Status::NoContent:
        return nullopt;
    case Status::Ok: {
        const auto sub_resp = handle_server_resp(resp);
        if (sub_resp.has_value()) {
            conn_factory.invalidate();
        }
        return sub_resp;
    }
    default:
        conn_factory.invalidate();
        return Error::UnexpectedResponse;
    }
}

void connect::run() {
    CONNECT_DEBUG("%s", "Connect client starts\n");
    // waits for file-system and network interface to be ready
    //FIXME! some mechanisms to know that file-system and network are ready.
    osDelay(10000);

    while (true) {
        // TODO: Deal with the error somehow
        communicate();
        // Connect server expects telemetry at least every 30 s (varies with design decisions).
        // So the client has to communicate very frequently with the server!
        osDelay(10000);
    }
}

connect::connect()
    : printer_info(core.get_printer_info()) {}

}
