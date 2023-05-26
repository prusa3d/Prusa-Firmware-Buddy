#include "connect.hpp"
#include <http/httpc.hpp>
#include "tls/tls.hpp"
#include "command_id.hpp"
#include "segmented_json.h"
#include "render.hpp"
#include "json_out.hpp"
#include "connection_cache.hpp"

#include <log.h>

#include <atomic>
#include <cassert>
#include <cstring>
#include <debug.h>
#include <cstring>
#include <optional>
#include <variant>

using namespace http;
using json::ChunkRenderer;
using json::JsonRenderer;
using json::JsonResult;
using std::decay_t;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::is_same_v;
using std::monostate;
using std::move;
using std::nullopt;
using std::optional;
using std::string_view;
using std::variant;
using std::visit;

LOG_COMPONENT_DEF(connect, LOG_SEVERITY_DEBUG);

namespace connect_client {

namespace {

    std::atomic<OnlineStatus> last_known_status = OnlineStatus::Unknown;
    std::atomic<bool> registration = false;
    std::atomic<const char *> registration_code_ptr = nullptr;

    OnlineStatus err_to_status(const Error error) {
        switch (error) {
        case Error::Connect:
            return OnlineStatus::NoConnection;
        case Error::Dns:
            return OnlineStatus::NoDNS;
        case Error::InternalError:
        case Error::ResponseTooLong:
        case Error::SetSockOpt:
            return OnlineStatus::InternalError;
        case Error::Network:
        case Error::Timeout:
            return OnlineStatus::NetworkError;
        case Error::Parse:
            return OnlineStatus::Confused;
        case Error::Tls:
            return OnlineStatus::Tls;
        default:
            return OnlineStatus::Unknown;
        }
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
        BasicRequest(Printer &printer, const Printer::Config &config, const Action &action, Tracked &telemetry_changes, optional<CommandId> background_command_id)
            : hdrs {
                // Even though the fingerprint is on a temporary, that
                // pointer is guaranteed to stay stable.
                { "Fingerprint", printer.printer_info().fingerprint, Printer::PrinterInfo::FINGERPRINT_HDR_SIZE },
                { "Token", config.token, nullopt },
                { nullptr, nullptr, nullopt }
            }
            , renderer_impl(RenderState(printer, action, telemetry_changes, background_command_id))
            , target_url(visit([](const auto &action) { return url(action); }, action)) {}
        virtual const char *url() const override {
            return target_url;
        }
        virtual const HeaderOut *extra_headers() const override {
            return hdrs;
        }
    };

    // TODO: We probably want to be able to both have a smaller buffer and
    // handle larger responses. We need some kind of parse-as-it-comes approach
    // for that.
    const constexpr size_t MAX_RESP_SIZE = 256;

    // Send a full telemetry every 5 minutes.
    const constexpr uint32_t FULL_TELEMETRY_EVERY = 5 * 60 * 1000;
}

Connect::ServerResp Connect::handle_server_resp(Response resp, CommandId command_id) {
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

optional<OnlineStatus> Connect::communicate(CachedFactory &conn_factory) {
    const auto [config, cfg_changed] = printer.config();

    // Make sure to reconnect if the configuration changes .
    if (cfg_changed) {
        conn_factory.invalidate();
        // Possibly new server, new telemetry cache...
        telemetry_changes.mark_dirty();
    }

    if (!config.enabled) {
        planner().reset();
        Sleep::idle().perform(printer, planner());
        return OnlineStatus::Off;
    } else if (config.host[0] == '\0' || config.token[0] == '\0') {
        planner().reset();
        Sleep::idle().perform(printer, planner());
        return OnlineStatus::NoConfig;
    } else if (status_replace_early(last_known_status)) {
        last_known_status = OnlineStatus::Connecting;
    }

    printer.drop_paths(); // In case they were left in there in some early-return case.
    auto borrow = buffer.borrow();
    if (planner().wants_job_paths()) {
        assert(borrow.has_value());
    } else {
        borrow.reset();
    }
    printer.renew(move(borrow));

    auto action = planner().next_action(buffer);

    // Handle sleeping first. That one doesn't need the connection.
    if (auto *s = get_if<Sleep>(&action)) {
        s->perform(printer, planner());
        return nullopt;
    } else if (auto *e = get_if<Event>(&action); e && e->type == EventType::Info) {
        // The server may delete its latest copy of telemetry in various case, in particular:
        // * When it thinks we were offline for a while.
        // * When it went through an update.
        //
        // In either case, we send or the server asks us to send the INFO
        // event. We may send INFO for other reasons too, but don't bother to
        // make that distinction for simplicity.
        telemetry_changes.mark_dirty();
    }

    // Let it reconnect if it needs it.
    conn_factory.refresh(config);

    HttpClient http(conn_factory);

    uint32_t start = now();
    // Underflow should naturally work
    if (start - last_full_telemetry >= FULL_TELEMETRY_EVERY) {
        // The server wants to get a full telemetry from time to time, despite
        // it not being changed. Some caching reasons/recovery/whatever?
        //
        // If we didn't send a new telemetry for too long, reset the
        // fingerprint, which'll trigger the resend.
        telemetry_changes.mark_dirty();
    }

    const auto background_command_id = planner().background_command_id();

    BasicRequest request(printer, config, action, telemetry_changes, background_command_id);
    ExtractCommanId cmd_id;
    const auto result = http.send(request, &cmd_id);
    // Drop current job paths (if any) to make space for potentially parsing a command from the server.
    // In case we failed to send the JOB_INFO event that uses the paths, we
    // will acquire it and fill it in the next iteration anyway.
    //
    // Note that this invalidates the paths inside params in the current printer snapshot.
    printer.drop_paths();

    if (holds_alternative<Error>(result)) {
        planner().action_done(ActionResult::Failed);
        conn_factory.invalidate();
        return err_to_status(get<Error>(result));
    }

    Response resp = get<Response>(result);
    if (!resp.can_keep_alive) {
        conn_factory.invalidate();
    }
    const bool is_full_telemetry = holds_alternative<SendTelemetry>(action) && !get<SendTelemetry>(action).empty;
    switch (resp.status) {
    // The server has nothing to tell us
    case Status::NoContent:
        planner().action_done(ActionResult::Ok);
        if (is_full_telemetry && telemetry_changes.is_dirty()) {
            // We check the is_dirty too, because if it was _not_ dirty, we
            // sent only partial telemetry and don't want to reset the
            // last_full_telemetry.
            telemetry_changes.mark_clean();
            last_full_telemetry = start;
        }
        return OnlineStatus::Ok;
    case Status::Ok: {
        if (is_full_telemetry && telemetry_changes.is_dirty()) {
            // Yes, even before checking the command we got is OK. We did send
            // the telemetry, what happens to the command doesn't matter.
            telemetry_changes.mark_clean();
            last_full_telemetry = start;
        }
        if (cmd_id.command_id.has_value()) {
            const auto sub_resp = handle_server_resp(resp, *cmd_id.command_id);
            return visit([&](auto &&arg) -> optional<OnlineStatus> {
                // Trick out of std::visit documentation. Switch by the type of arg.
                using T = decay_t<decltype(arg)>;

                if constexpr (is_same_v<T, monostate>) {
                    planner().action_done(ActionResult::Ok);
                    return OnlineStatus::Ok;
                } else if constexpr (is_same_v<T, Command>) {
                    planner().action_done(ActionResult::Ok);
                    planner().command(arg);
                    return OnlineStatus::Ok;
                } else if constexpr (is_same_v<T, Error>) {
                    planner().action_done(ActionResult::Failed);
                    planner().command(Command {
                        cmd_id.command_id.value(),
                        BrokenCommand {},
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
            return OnlineStatus::Confused;
        }
    }
    case Status::RequestTimeout:
    case Status::TooManyRequests:
    case Status::ServiceTemporarilyUnavailable:
    case Status::GatewayTimeout:
        conn_factory.invalidate();
        // These errors are likely temporary and will go away eventually.
        planner().action_done(ActionResult::Failed);
        return OnlineStatus::ServerError;
    default:
        conn_factory.invalidate();
        // We don't know that exactly the server answer means, but we guess
        // that it will persist, so we consider it refused and throw the
        // request away.
        planner().action_done(ActionResult::Refused);
        // Switch just to provide proper error message
        switch (resp.status) {
        case Status::BadRequest:
            return OnlineStatus::InternalError;
        case Status::Forbidden:
        case Status::Unauthorized:
            return OnlineStatus::Auth;
        default:
            return OnlineStatus::ServerError;
        }
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
            if (new_status.has_value()) {
                last_known_status = *new_status;
            }
        } else if (reg_wanted && !reg_running) {
            guts.emplace<Registrator>(printer);
            last_known_status = OnlineStatus::Unknown;
            conn_factory.invalidate();
            registration_code_ptr = get<Registrator>(guts).get_code();
        } else if (!reg_wanted && reg_running) {
            last_known_status = OnlineStatus::Unknown;
            registration_code_ptr = nullptr;
            guts.emplace<Planner>(printer);
            conn_factory.invalidate();
        } else {
            const auto new_status = communicate(conn_factory);
            if (new_status.has_value()) {
                last_known_status = *new_status;
            }
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
    return last_known_status;
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
    if (status == OnlineStatus::RegistrationCode || status == OnlineStatus::RegistrationDone) {
        return registration_code_ptr;
    } else {
        return nullptr;
    }
}

}
