#pragma once

#include "changes.hpp"
#include "planner.hpp"
#include "printer.hpp"
#include "registrator.hpp"

#include <http/httpc.hpp>
#include <common/shared_buffer.hpp>
#include <option/websocket.h>
#if WEBSOCKET()
    #include <common/http/websocket.hpp>
#endif

namespace connect_client {

class CachedFactory;

OnlineStatus last_status();

bool is_connect_registered();

/*
 * Registration process.
 *
 * The GUI (or whatever) thread calls `request_registration` and observes the `last_status`.
 *
 * * Once the last_status shows RegistrationCode, it can get the code by `registration_code`.
 * * Once the user uses the code, it goes to `RegistrationDone`.
 * * The GUI thread is responsible to calling leave_registration (in all cases,
 *   even if RegistrationDone or RegistrationError was reached).
 */
void request_registration();
// May be used to abort registration at any time, or confirm finishing.
void leave_registration();

/*
 * Provides the registration code.
 *
 * This may be called only if last_status returned RegistrationCode or
 * RegistrationDone, by the thread that did request_registration().
 *
 * The returned pointer is valid only until the thread calls
 * leave_registration(). The content (the string) won't change during the time.
 */
const char *registration_code();

enum class RequestType {
    Telemetry,
    SendInfo,
};

class Connect {
private:
    std::variant<Planner, Registrator> guts;
    Printer &printer;
    SharedBuffer &buffer;

    using ServerResp = std::variant<std::monostate, Command, http::Error>;

    Planner &planner();

#if WEBSOCKET()
    // FIXME:
    // This is a temporary place, to allow as minimal difference between
    // websocket and "old" style communication. It is also a bit _buggy_ (it
    // might allow using the websocketed connection in the registrator for
    // HTTP, which won't work). Eventually, this'll get integrated into the
    // factory, but for now and for the experiments, we have it separate.
    std::optional<http::WebSocket> websocket;
    // Time when we sent something last, in ms.
    // Used to track when we should generate a ping.
    uint32_t last_send = 0;

    CommResult receive_command(CachedFactory &conn_factory);
    CommResult send_ping(CachedFactory &conn_factory);
#endif

    CommResult prepare_connection(CachedFactory &conn_factory, const Printer::Config &config);
    CommResult send_command(CachedFactory &conn_factory, const Printer::Config &config, Action &&action, std::optional<CommandId> background_command_id);
    // transmission and reception with Connect server
    CommResult communicate(CachedFactory &conn_factory);
    ServerResp handle_server_resp(http::Response response, CommandId command_id);
    Connect(const Connect &other) = delete;
    Connect(Connect &&other) = delete;

public:
    Connect(Printer &printer, SharedBuffer &buffer);
    void run(void) __attribute__((noreturn));
};

} // namespace connect_client
