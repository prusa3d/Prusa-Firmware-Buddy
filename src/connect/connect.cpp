#include "connect.hpp"
#include "tls/tls.hpp"
#include "command_id.hpp"
#include "segmented_json.h"
#include "render.hpp"
#include "json_out.hpp"
#include "connection_cache.hpp"

#include <http/httpc.hpp>
#include <http/websocket.hpp>

#include <log.h>

#include <atomic>
#include <cassert>
#include <cstring>
#include <debug.h>
#include <cstring>
#include <optional>
#include <variant>
#include <charconv>

using namespace http;
using json::ChunkRenderer;
using json::JsonRenderer;
using json::JsonResult;
using std::decay_t;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::is_same_v;
using std::make_optional;
using std::make_tuple;
using std::monostate;
using std::move;
using std::nullopt;
using std::optional;
using std::string_view;
using std::variant;
using std::visit;

LOG_COMPONENT_DEF(connect, LOG_SEVERITY_INFO);

namespace connect_client {

namespace {

    // These two should actually be a atomic<tuple<.., ..>>. This won't compile on our platform.
    // But, considering the error is informative only and we set these only in
    // this thread, any temporary inconsistency in them is of no concern
    // anyway, so they can be two separate atomics without any adverse effects.
    std::atomic<ConnectionStatus> last_known_status = ConnectionStatus::Unknown;
    std::atomic<OnlineError> last_connection_error = OnlineError::NoError;
    std::atomic<optional<uint8_t>> retries_left;

    std::atomic<bool> registration = false;
    std::atomic<const char *> registration_code_ptr = nullptr;

    void process_status(monostate, ConnectionStatus) {}

    void process_status(OnlineError error, ConnectionStatus err_status) {
        last_known_status = err_status;
        last_connection_error = error;
    }

    void process_status(ConnectionStatus status, ConnectionStatus /* err_status unused */) {
        last_known_status = status;
        switch (status) {
        case ConnectionStatus::Ok:
        case ConnectionStatus::NoConfig:
        case ConnectionStatus::Off:
        case ConnectionStatus::RegistrationCode:
        case ConnectionStatus::RegistrationDone:
            // These are the states we want to stay in, if we are in one,
            // any past errors make no sense, we are happy.
            last_connection_error = OnlineError::NoError;
            retries_left = nullopt;
            break;
        default:
            break;
        }
    }

    void process_status(ErrWithRetry err, ConnectionStatus err_status) {
        last_connection_error = err.err;
        retries_left = err.retry;
        if (err.retry == 0) {
            last_known_status = err_status;
        }
    }

    void process_status(CommResult status, ConnectionStatus err_status) {
        visit([&](auto s) { process_status(s, err_status); }, status);
    }

    class BasicRequest final : public JsonPostRequest {
    private:
        HeaderOut hdrs[3];
        Renderer renderer_impl;
        const char *target_url;
        static const char *url(const Sleep &) {
            // Sleep already handled at upper level.
            assert(0);
            return "";
        }
        static const char *url(const ReadCommand &) {
            // Not used in non-websocket mode
            assert(0);
            return "";
        }
        static const char *url(const SendTelemetry &) {
            return "/p/telemetry";
        }
        static const char *url(const Event &) {
            return "/p/events";
        }

    protected:
        virtual ChunkRenderer &renderer() override {
            return renderer_impl;
        }

    public:
        BasicRequest(Printer &printer, const Printer::Config &config, const Action &action, optional<CommandId> background_command_id)
            : hdrs {
                // Even though the fingerprint is on a temporary, that
                // pointer is guaranteed to stay stable.
                { "Fingerprint", printer.printer_info().fingerprint, Printer::PrinterInfo::FINGERPRINT_HDR_SIZE },
                { "Token", config.token, nullopt },
                { nullptr, nullptr, nullopt }
            }
            , renderer_impl(RenderState(printer, action, background_command_id))
            , target_url(visit([](const auto &action) { return url(action); }, action)) {}
        virtual const char *url() const override {
            return target_url;
        }
        virtual const HeaderOut *extra_headers() const override {
            return hdrs;
        }
    };

    class UpgradeRequest final : public http::Request {
    private:
        // 2 for auth
        // 1 for upgrade
        // 3 for websocket negotiation
        // 1 for sentinel
        HeaderOut hdrs[7];

    public:
        UpgradeRequest(Printer &printer, const Printer::Config &config, const WebSocketKey &key)
            : hdrs {
                // Even though the fingerprint is on a temporary, that
                // pointer is guaranteed to stay stable.
                { "Fingerprint", printer.printer_info().fingerprint, Printer::PrinterInfo::FINGERPRINT_HDR_SIZE },
                { "Token", config.token, nullopt },
                { "Upgrade", "websocket", nullopt },
                { "Sec-WebSocket-Key", key.req(), nullopt },
                { "Sec-WebSocket-Version", "13", nullopt },
                { "Sec-WebSocket-Protocol", "prusa-connect", nullopt },
                { nullptr, nullptr, nullopt }
            } {}
        virtual const char *url() const override {
            return "/p/ws-pre";
        }
        virtual Method method() const override {
            return Method::Get;
        }
        virtual ContentType content_type() const override {
            // Not actually used for a get request
            return ContentType::ApplicationOctetStream;
        }
        virtual const HeaderOut *extra_headers() const override {
            return hdrs;
        }

        virtual const char *connection() const override {
            return "upgrade";
        }
    };

    // TODO: We probably want to be able to both have a smaller buffer and
    // handle larger responses. We need some kind of parse-as-it-comes approach
    // for that.
    // Note: This buffer is huge, but we are in the shallow waters of the stack, so it
    // should be fine, even 3200 buffer still did not overflow in my tests, even in debug.
    // So unless the call stack changes significantly, we are fine, just beware if doing
    // larger changes, or using this elsewhere.
    const constexpr size_t MAX_RESP_SIZE = 512;

    // Send a full telemetry every 5 minutes.
    const constexpr Duration FULL_TELEMETRY_EVERY = 5 * 60 * 1000;

    // Wait this long between config change checking when Connect is off.
    const constexpr Duration CONFIG_ENABLED_RECHECK = 1000;
} // namespace

Connect::ServerResp Connect::handle_server_resp(http::Response resp, CommandId command_id) {
    // TODO We want to make this buffer smaller, eventually. In case of custom
    // gcode, we can load directly into the shared buffer. In case of JSON, we
    // want to implement stream/iterative parsing.

    // Note: Reading the body early, before some checks. That's before we want
    // to consume it even in case we don't need it, because we want to reuse
    // the http connection and the next response would get confused by
    // leftovers.
    uint8_t recv_buffer[MAX_RESP_SIZE];
    const auto result = resp.read_all(recv_buffer, sizeof recv_buffer);

    if (auto *err = get_if<Error>(&result); err != nullptr) {
        return *err;
    }
    size_t size = get<size_t>(result);

    if (command_id == planner().background_command_id()) {
        return Command {
            command_id,
            ProcessingThisCommand {},
        };
    }

    auto buff(buffer.borrow());
    if (!buff.has_value()) {
        // We can only hold the buffer already borrowed in case we are still
        // processing some command. In that case we can't accept another one
        // and we just reject it.
        return Command {
            command_id,
            ProcessingOtherCommand {},
        };
    }

    // Note: Anything of these can result in an "Error"-style command (Unknown,
    // Broken...). Nevertheless, we return a Command, which'll consider the
    // whole request-response pair a successful one. That's OK, because on the
    // lower-level it is - we consumed all the data and are allowed to reuse
    // the connection and all that.
    switch (resp.content_type) {
    case ContentType::TextGcode: {
        const string_view body(reinterpret_cast<const char *>(recv_buffer), size);
        return Command::gcode_command(command_id, body, move(*buff));
    }
    case ContentType::ApplicationJson:
        return Command::parse_json_command(command_id, reinterpret_cast<char *>(recv_buffer), size, move(*buff));
    default:;
        // If it's unknown content type, then it's unknown command because we
        // have no idea what to do about it / how to even parse it.
        return Command {
            command_id,
            UnknownCommand {},
        };
    }
}

#if WEBSOCKET()
CommResult Connect::receive_command(CachedFactory &conn_factory) {
    log_debug(connect, "Trying to receive a commandfrom server");
    bool first = true;
    bool more = true;
    size_t read = 0;
    bool oversized = false;
    // The extra 9 are for the message "header"
    // 1 is for "type", 8 are 32bit command ID in hex (without 0x in front).
    constexpr size_t HDR_LEN = 9;
    uint8_t buffer[MAX_RESP_SIZE + HDR_LEN];
    while (more) {
        auto res = websocket->receive(first ? make_optional(0) : nullopt);

        if (holds_alternative<monostate>(res)) {
            log_debug(connect, "No command available now");
            break;
        } else if (holds_alternative<Error>(res)) {
            log_error(connect, "Lost connection when reading command");
            conn_factory.invalidate();
            return err_to_status(get<Error>(res));
        }

        auto fragment = get<WebSocket::FragmentHeader>(res);

        // Control messages can come at any time. Even "in the middle" of
        // multi-fragment message.
        switch (fragment.opcode) {
        case WebSocket::Opcode::Ping: {
            log_debug(connect, "Ping from server");
            // Not allowed to fragment
            constexpr const size_t MAX_FRAGMENT_LEN = 126;
            if (fragment.len > MAX_FRAGMENT_LEN) {
                conn_factory.invalidate();
                return OnlineError::Confused;
            }

            uint8_t data[MAX_FRAGMENT_LEN];
            if (auto error = fragment.conn->rx_exact(data, fragment.len); error.has_value()) {
                conn_factory.invalidate();
                return err_to_status(*error);
            }

            if (auto error = websocket->send(WebSocket::Pong, false, data, fragment.len); error.has_value()) {
                conn_factory.invalidate();
                return err_to_status(*error);
            }

            // This one is handled, next one please.
            continue;
        }
        case WebSocket::Opcode::Pong:
            // We didn't send a ping, so not expecting pong... ignore pongs
            fragment.ignore();
            continue;
        case WebSocket::Opcode::Close:
            // The server is closing the connection, we are not getting the
            // message. Throw the connection away.
            conn_factory.invalidate();
            log_info(connect, "Close from server");
            return OnlineError::Network;
        default:
            // It's not websocket control message, handle it below
            break;
        }

        first = false;

        // Even if we know it won't fit, we make sure to read at least the
        // header, so we have a command ID to refuse.
        size_t to_read = std::min(sizeof buffer - read, fragment.len);
        if (auto error = fragment.conn->rx_exact(buffer + read, to_read); error.has_value()) {
            conn_factory.invalidate();
            return err_to_status(*error);
        }
        read += to_read;
        fragment.len -= to_read;
        if (fragment.len > 0) {
            oversized = true;
            // As we decreased the size already, this ignores only the rest.
            fragment.ignore();
        }

        if (fragment.last) {
            if (read < HDR_LEN) {
                planner().command(Command {
                    // We don't have a command ID, so we cheat a bit here. Should not happen really.
                    0,
                    BrokenCommand { "Message too short to contain header" } });
                return ConnectionStatus::Ok;
            }
            uint32_t command_id;
            auto id_result = std::from_chars(reinterpret_cast<const char *>(buffer + 1), reinterpret_cast<const char *>(buffer + 9), command_id, 16);
            if (id_result.ec != std::errc {}) {
                planner().command(Command {
                    0,
                    BrokenCommand { "Could not parse command ID" } });
                return ConnectionStatus::Ok;
            }
            log_debug(connect, "Received a command from server");
            enum class Type {
                Json,
                Gcode,
                ForcedGcode,
            };
            Type type;
            switch (buffer[0]) {
            case 'J':
                type = Type::Json;
                break;
            case 'G':
                type = Type::Gcode;
                break;
            case 'F':
                type = Type::ForcedGcode;
                break;
            case 's':
                // The `s` was used in some experiments. It's not actually part
                // of the protocol right now, but it's ignored temporarily to
                // deal with version transitions. Shall be removed before we
                // put this to production.
                return ConnectionStatus::Ok;
            case 'D':
                // This is a debug message. We just log it and throw away (and
                // we don't care about the oversized).
                log_debug(connect, "Msg from server: %.*s", static_cast<int>(read - HDR_LEN), reinterpret_cast<const char *>(buffer + HDR_LEN));
                return ConnectionStatus::Ok;
            default:
                planner().command(Command {
                    command_id,
                    BrokenCommand { "Unrecognized type of message" } });
                return ConnectionStatus::Ok;
            }

            if (oversized) {
                planner().command(Command {
                    command_id,
                    BrokenCommand { "Command too large" } });

                // We have managed to consume the command alright, we just
                // didn't handle it -> the connection itself is fine.
                return ConnectionStatus::Ok;
            }
            if (command_id == planner().background_command_id()) {
                planner().command(Command {
                    command_id,
                    ProcessingThisCommand {},
                });

                return ConnectionStatus::Ok;
            }
            auto buff(this->buffer.borrow());
            if (!buff.has_value()) {
                // We can only hold the buffer already borrowed in case we are still
                // processing some command. In that case we can't accept another one
                // and we just reject it.
                planner().command(Command {
                    command_id,
                    ProcessingOtherCommand {},
                });
            }

            switch (type) {
            case Type::Json: {
                auto command = Command::parse_json_command(command_id, reinterpret_cast<char *>(buffer + HDR_LEN), read - HDR_LEN, move(*buff));
                planner().command(command);
                break;
            }
            case Type::Gcode:
            case Type::ForcedGcode: {
                // Forced GCode: The HTTP version has a `Force` header. The
                // idea is we would refuse non-forced gcode during a print, but
                // accept the forced one. We don't have that distinction
                // implemented, but the websocket protocol needs to put the
                // distinction somewhere and we need to understand both the `G`
                // and `F` types.
                //
                // TODO: We should implement the distinction O:-)
                const string_view body(reinterpret_cast<const char *>(buffer + HDR_LEN), read - HDR_LEN);
                auto command = Command::gcode_command(command_id, body, move(*buff));
                break;
            }
            }

            more = false;
        }
    }

    return ConnectionStatus::Ok;
}

CommResult Connect::prepare_connection(CachedFactory &conn_factory, const Printer::Config &config) {
    if (!conn_factory.is_valid()) {
        // With websocket, we don't try the connection if we are in the error
        // state, so get rid of potential error state first.
        conn_factory.invalidate();

        // Could have been using the old connection and contain a dangling pointer. Get rid of it.
        // (We currently don't do a proper shutdown
        websocket.reset();
        last_known_status = ConnectionStatus::Connecting;
    }
    // Let it reconnect if it needs it.
    conn_factory.refresh(config);

    HttpClient http(conn_factory);

    if (conn_factory.is_valid() && !websocket.has_value()) {
        // Let's do the upgrade

        WebSocketKey websocket_key;
        WebSocketAccept upgrade_hdrs(websocket_key);
        UpgradeRequest upgrade(printer, config, websocket_key);
        const auto result = http.send(upgrade, &upgrade_hdrs);
        if (holds_alternative<Error>(result)) {
            conn_factory.invalidate();
            return err_to_status(get<Error>(result));
        }

        auto resp = get<http::Response>(result);
        switch (resp.status) {
        case Status::SwitchingProtocols: {
            if (!upgrade_hdrs.key_matched() || !upgrade_hdrs.all_supported()) {
                conn_factory.invalidate();
                return OnlineError::Server;
            }

            // Read and throw away the body, if any. Not interesting.
            uint8_t throw_away[128];
            size_t received = 0;
            do {
                auto result = resp.read_body(throw_away, sizeof throw_away);
                if (holds_alternative<Error>(result)) {
                    conn_factory.invalidate();
                    return err_to_status(get<Error>(result));
                }
                received = get<size_t>(result);
            } while (received > 0);

            websocket = WebSocket::from_response(resp);
            break;
        }
        default: {
            conn_factory.invalidate();
            planner().action_done(ActionResult::Refused);
            switch (resp.status) {
            case Status::BadRequest:
                return OnlineError::Internal;
            case Status::Forbidden:
            case Status::Unauthorized:
                return OnlineError::Auth;
            default:
                return OnlineError::Server;
            }
            break;
        }
        }
    }

    return monostate {};
}

CommResult Connect::send_command(CachedFactory &conn_factory, const Printer::Config &, Action &&action, optional<CommandId> background_command_id) {
    log_debug(connect, "Sending to server");
    if (!websocket.has_value()) {
        planner().action_done(ActionResult::Failed);
        return OnlineError::Network;
    }
    Renderer renderer(RenderState(printer, action, background_command_id));
    uint8_t buffer[MAX_RESP_SIZE];
    bool more = true;
    bool first = true;
    while (more) {
        const auto [result, written_json] = renderer.render(buffer, sizeof buffer);
        switch (result) {
        case JsonResult::Abort:
        case JsonResult::BufferTooSmall:
            return OnlineError::Internal;
        case JsonResult::Complete:
            more = false;
            break;
        case JsonResult::Incomplete:
            break;
        }

        if (auto error = websocket->send(first ? WebSocket::Text : WebSocket::Continuation, !more, buffer, written_json); error.has_value()) {
            conn_factory.invalidate();
            planner().action_done(ActionResult::Failed);
            return err_to_status(*error);
        }

        first = false;
    }

    log_debug(connect, "Sending done");

    planner().action_done(ActionResult::Ok);

    return ConnectionStatus::Ok;
}
#else
CommResult Connect::prepare_connection(CachedFactory &conn_factory, const Printer::Config &config) {
    if (!conn_factory.is_valid()) {
        last_known_status = ConnectionStatus::Connecting;
    }
    // Let it reconnect if it needs it.
    conn_factory.refresh(config);

    return monostate {};
}

CommResult Connect::send_command(CachedFactory &conn_factory, const Printer::Config &config, Action &&action, optional<CommandId> background_command_id) {
    log_debug(connect, "Sending to connect");
    BasicRequest request(printer, config, action, background_command_id);
    ExtractCommanId cmd_id;

    HttpClient http(conn_factory);

    const auto result = http.send(request, &cmd_id);
    // Drop current job paths (if any) to make space for potentially parsing a command from the server.
    // In case we failed to send the JOB_INFO event that uses the paths, we
    // will acquire it and fill it in the next iteration anyway.
    //
    // Note that this invalidates the paths inside params in the current printer snapshot.
    printer.drop_paths();

    if (holds_alternative<Error>(result)) {
        log_debug(connect, "Failed to exchange");
        planner().action_done(ActionResult::Failed);
        conn_factory.invalidate();
        return err_to_status(get<Error>(result));
    }

    http::Response resp = get<http::Response>(result);
    log_debug(connect, "Response with status %" PRIu16, static_cast<uint16_t>(resp.status));
    if (!resp.can_keep_alive) {
        conn_factory.invalidate();
    }
    switch (resp.status) {
    // The server has nothing to tell us
    case Status::NoContent:
        log_debug(connect, "Have a response without body");
        planner().action_done(ActionResult::Ok);
        return ConnectionStatus::Ok;
    case Status::Ok: {
        log_debug(connect, "Have a response with body");
        if (cmd_id.command_id.has_value()) {
            const auto sub_resp = handle_server_resp(resp, *cmd_id.command_id);
            return visit([&](auto &&arg) -> CommResult {
                // Trick out of std::visit documentation. Switch by the type of arg.
                using T = decay_t<decltype(arg)>;

                if constexpr (is_same_v<T, monostate>) {
                    planner().action_done(ActionResult::Ok);
                    return ConnectionStatus::Ok;
                } else if constexpr (is_same_v<T, Command>) {
                    planner().action_done(ActionResult::Ok);
                    planner().command(arg);
                    return ConnectionStatus::Ok;
                } else if constexpr (is_same_v<T, Error>) {
                    planner().action_done(ActionResult::Failed);
                    planner().command(Command {
                        cmd_id.command_id.value(),
                        BrokenCommand { to_str(arg) },
                    });
                    conn_factory.invalidate();
                    return err_to_status(arg);
                }
            },
                sub_resp);
        } else {
            // We have received a command without command ID
            // There's no better action for us than just throw it away.
            planner().action_done(ActionResult::Refused);
            conn_factory.invalidate();
            return OnlineError::Confused;
        }
    }
    case Status::RequestTimeout:
    case Status::TooManyRequests:
    case Status::ServiceTemporarilyUnavailable:
    case Status::GatewayTimeout:
        conn_factory.invalidate();
        // These errors are likely temporary and will go away eventually.
        planner().action_done(ActionResult::Failed);
        return OnlineError::Server;
    default:
        conn_factory.invalidate();
        // We don't know that exactly the server answer means, but we guess
        // that it will persist, so we consider it refused and throw the
        // request away.
        planner().action_done(ActionResult::Refused);
        // Switch just to provide proper error message
        switch (resp.status) {
        case Status::BadRequest:
            return OnlineError::Internal;
        case Status::Forbidden:
        case Status::Unauthorized:
            return OnlineError::Auth;
        default:
            return OnlineError::Server;
        }
    }
}
#endif

CommResult Connect::communicate(CachedFactory &conn_factory) {
    const auto [config, cfg_changed] = printer.config();

    // Make sure to reconnect if the configuration changes .
    if (cfg_changed) {
        conn_factory.invalidate();
        // Possibly new server
        planner().reset();
    }

    if (!config.enabled) {
        planner().reset();
        Sleep::idle(CONFIG_ENABLED_RECHECK).perform(printer, planner());
        return ConnectionStatus::Off;
    } else if (config.host[0] == '\0' || config.token[0] == '\0') {
        planner().reset();
        Sleep::idle(CONFIG_ENABLED_RECHECK).perform(printer, planner());
        return ConnectionStatus::NoConfig;
    }

    printer.drop_paths(); // In case they were left in there in some early-return case.
    auto borrow = buffer.borrow();
    if (planner().wants_job_paths()) {
        assert(borrow.has_value());
    } else {
        borrow.reset();
    }
    printer.renew(move(borrow));

    // This is a bit of a hack, we want to keep watching for USB being inserted
    // or not. We don't have a good place, so we stuck it here.
    transfers::ChangedPath::instance.media_inserted(printer.params().has_usb);

    Connection *wake_on_readable = nullptr;
#if WEBSOCKET()
    // The "ordinary" http exchange gets data in the response, websocket at any
    // time so we want to let it get woken up by arriving data in sleep (the
    // planner checks if it _can_ receive the command at that point).
    if (conn_factory.is_valid() && websocket.has_value()) {
        wake_on_readable = websocket->inner_connection();
    }
#endif
    auto action = planner().next_action(buffer, wake_on_readable);

    // Handle sleeping first. That one doesn't need the connection.
    if (auto *s = get_if<Sleep>(&action)) {
        s->perform(printer, planner());
        return monostate {};
    }

    auto prepared = prepare_connection(conn_factory, config);
    if (!holds_alternative<monostate>(prepared)) {
        log_debug(connect, "No connection to communicate");
        return prepared;
    }

    const auto background_command_id = planner().background_command_id();

    if (holds_alternative<ReadCommand>(action)) {
#if WEBSOCKET()
        return receive_command(conn_factory);
#else
        assert(0); // Doesn't happen in non-websocket mode, commands come in responses
        return err_to_status(Error::InternalError);
#endif
    } else {
        return send_command(conn_factory, config, move(action), background_command_id);
    }
}

void Connect::run() {
    log_debug(connect, "%s", "Connect client starts\n");

    CachedFactory conn_factory;

    while (true) {
        auto reg_wanted = registration.load();
        auto reg_running = holds_alternative<Registrator>(guts);
        if (reg_wanted && reg_running) {
            const auto new_status = get<Registrator>(guts).communicate(conn_factory);
            process_status(new_status, ConnectionStatus::RegistrationError);
        } else if (reg_wanted && !reg_running) {
            guts.emplace<Registrator>(printer);
            last_known_status = ConnectionStatus::Unknown;
            last_connection_error = OnlineError::NoError;
            retries_left = nullopt;
            conn_factory.invalidate();
            registration_code_ptr = get<Registrator>(guts).get_code();
        } else if (!reg_wanted && reg_running) {
            last_known_status = ConnectionStatus::Unknown;
            last_connection_error = OnlineError::NoError;
            retries_left = nullopt;
            registration_code_ptr = nullptr;
            guts.emplace<Planner>(printer);
            conn_factory.invalidate();
        } else {
            const auto new_status = communicate(conn_factory);
            process_status(new_status, ConnectionStatus::Error);
        }
    }
}

Planner &Connect::planner() {
    assert(holds_alternative<Planner>(guts));
    return get<Planner>(guts);
}

Connect::Connect(Printer &printer, SharedBuffer &buffer)
    : guts(Planner(printer))
    , printer(printer)
    , buffer(buffer) {}

OnlineStatus last_status() {
    return make_tuple(last_known_status.load(), last_connection_error.load(), retries_left.load());
}

void request_registration() {
    bool old = registration.exchange(true);
    // Avoid warnings
    (void)old;
    assert(!old);
}

void leave_registration() {
    bool old = registration.exchange(false);
    // Avoid warnings
    (void)old;
    assert(old);
}

const char *registration_code() {
    // Note: This is just a safety, the caller shall not call us in case this
    // is not the case.
    const auto status = last_known_status.load();
    if (status == ConnectionStatus::RegistrationCode || status == ConnectionStatus::RegistrationDone) {
        return registration_code_ptr;
    } else {
        return nullptr;
    }
}

} // namespace connect_client
