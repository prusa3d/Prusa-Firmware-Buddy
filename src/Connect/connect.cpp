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

using http::ContentType;
using http::Status;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::nullopt;
using std::optional;
using std::variant;
using std::visit;

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
        Action &action;
        HeaderOut hdrs[3];
        bool done = false;
        using RenderResult = variant<size_t, Error>;
        const char *url(const Sleep &) const {
            // Sleep already handled at upper level.
            assert(0);
            return "";
        }
        const char *url(const SendTelemetry &) const {
            return "/p/telemetry";
        }
        const char *url(const Event &) const {
            return "/p/events";
        }

        RenderResult write_body_chunk(SendTelemetry &telemetry, char *data, size_t size) {
            if (telemetry.empty) {
                assert(size > 2);
                memcpy(data, "{}", 2);
                return 2;
            } else {
                device_params_t params = core.get_data();
                httpc_data renderer;
                return renderer.telemetry(params, data, size);
            }
        }

        RenderResult write_body_chunk(Event &event, char *data, size_t size) {
            // TODO: There are different kinds of events out there...
            httpc_data renderer;
            return renderer.info(info, data, size, 0);
        }

        Error write_body_chunk(Sleep &, char *, size_t) {
            // Handled at upper level.
            assert(0);
            return Error::INVALID_PARAMETER_ERROR;
        }

    public:
        BasicRequest(core_interface &core, const printer_info_t &info, const configuration_t &config, Action &action)
            : core(core)
            , info(info)
            , action(action)
            , hdrs {
                { "Fingerprint", info.fingerprint },
                { "Token", config.token },
                { nullptr, nullptr }
            } {}
        virtual const char *url() const override {
            return visit([&](auto &action) {
                return this->url(action);
            },
                action);
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
        virtual RenderResult write_body_chunk(char *data, size_t size) override {
            if (done) {
                return 0U;
            } else {
                done = true;
                return visit([&](auto &action) -> RenderResult {
                    return this->write_body_chunk(action, data, size);
                },
                    action);
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
        planner.reset();
        osDelay(10000);
        return nullopt;
    }

    auto action = planner.next_action();

    // Handle sleeping first. That one doesn't need the connection.
    if (auto *s = get_if<Sleep>(&action)) {
        osDelay(s->milliseconds);
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

    BasicRequest request(core, printer_info, config, action);
    const auto result = http.send(request);

    if (holds_alternative<Error>(result)) {
        planner.action_done(ActionResult::Failed);
        conn_factory.invalidate();
        return get<Error>(result);
    }

    Response resp = get<Response>(result);
    switch (resp.status) {
    // The server has nothing to tell us
    case Status::NoContent:
        planner.action_done(ActionResult::Ok);
        return nullopt;
    case Status::Ok: {
        const auto sub_resp = handle_server_resp(resp);
        if (sub_resp.has_value()) {
            planner.action_done(ActionResult::Failed);
            conn_factory.invalidate();
        } else {
            planner.action_done(ActionResult::Ok);
        }
        return sub_resp;
    }
    default:
        conn_factory.invalidate();
        // TODO: Figure out if the server somehow refused that instead
        // of failed to process.
        planner.action_done(ActionResult::Failed);
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
    }
}

connect::connect()
    : printer_info(core.get_printer_info()) {}

}
