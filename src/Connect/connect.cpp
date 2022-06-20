#include "connect.hpp"
#include "httpc.hpp"
#include "httpc_data.hpp"
#include "os_porting.hpp"
#include "tls/tls.hpp"
#include "socket.hpp"

#include <cmsis_os.h>
#include <log.h>

#include <cassert>
#include <debug.h>
#include <cstring>
#include <optional>
#include <variant>

using http::ContentType;
using http::Status;
using std::decay_t;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::is_same_v;
using std::min;
using std::monostate;
using std::nullopt;
using std::optional;
using std::string_view;
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
            // TODO: Incremental rendering support, if it doesn't fit into the buffer. Not yet supported.
            switch (event.type) {
            case EventType::Info: {
                httpc_data renderer;
                return renderer.info(info, data, size, event.command_id.value_or(0));
            }
            case EventType::Accepted:
            case EventType::Rejected: {
                // These events are always results of some commant we've received.
                // Checked when accepting the command.
                assert(event.command_id.has_value());
                size_t written = snprintf(data, size, "{\"event\":\"%s\",\"command_id\":%" PRIu32 "}", to_str(event.type), *event.command_id);

                // snprintf returns how much it would _want_ to write
                return min(size - 1 /* Taken up by the final \0 */, written);
            }
            default:
                assert(0);
                return 0;
            }
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

    // TODO: We probably want to be able to both have a smaller buffer and
    // handle larger responses. We need some kind of parse-as-it-comes approach
    // for that.
    const constexpr size_t MAX_RESP_SIZE = 256;
}

connect::ServerResp connect::handle_server_resp(Response resp) {
    if (resp.content_length() > MAX_RESP_SIZE) {
        return Error::ResponseTooLong;
    }

    // Note: missing command ID is already checked at upper level.
    CommandId command_id = resp.command_id.value();
    // XXX Use allocated string? Figure out a way to consume it in parts?
    uint8_t recv_buffer[MAX_RESP_SIZE];
    size_t pos = 0;

    while (resp.content_length() > 0) {
        const auto result = resp.read_body(recv_buffer + pos, resp.content_length());
        if (holds_alternative<size_t>(result)) {
            pos += get<size_t>(result);
        } else {
            return get<Error>(result);
        }
    }

    const string_view body(reinterpret_cast<const char *>(recv_buffer), pos);

    // Note: Anything of these can result in an "Error"-style command (Unknown,
    // Broken...). Nevertheless, we return a Command, which'll consider the
    // whole request-response pair a successful one. That's OK, because on the
    // lower-level it is - we consumed all the data and are allowed to reuse
    // the connection and all that.
    switch (resp.content_type) {
    case ContentType::TextGcode:
        return Command::gcode_command(command_id, body);
    case ContentType::ApplicationJson:
        return Command::parse_json_command(command_id, body);
    default:;
        // If it's unknown content type, then it's unknown command because we
        // have no idea what to do about it / how to even parse it.
        return Command {
            command_id,
            CommandType::Unknown,
        };
    }
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
        if (resp.command_id.has_value()) {
            const auto sub_resp = handle_server_resp(resp);
            return visit([&](auto &&arg) -> optional<Error> {
                // Trick out of std::visit documentation. Switch by the type of arg.
                using T = decay_t<decltype(arg)>;

                if constexpr (is_same_v<T, monostate>) {
                    planner.action_done(ActionResult::Ok);
                    return nullopt;
                } else if constexpr (is_same_v<T, Command>) {
                    planner.action_done(ActionResult::Ok);
                    planner.command(arg);
                    return nullopt;
                } else if constexpr (is_same_v<T, Error>) {
                    planner.action_done(ActionResult::Failed);
                    planner.command(Command {
                        resp.command_id.value(),
                        CommandType::Broken,
                    });
                    conn_factory.invalidate();
                    return arg;
                }
            },
                sub_resp);
        } else {
            // We have received a command without command ID
            // There's no better action for us than just throw it away.
            planner.action_done(ActionResult::Refused);
            conn_factory.invalidate();
            return Error::UnexpectedResponse;
        }
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
