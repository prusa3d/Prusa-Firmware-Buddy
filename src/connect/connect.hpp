#pragma once

#include "changes.hpp"
#include "planner.hpp"
#include "printer.hpp"
#include "registrator.hpp"
#include "status.hpp"

#include <http/httpc.hpp>
#include <common/shared_buffer.hpp>

namespace connect_client {

class CachedFactory;

OnlineStatus last_status();

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
    Tracked telemetry_changes;
    // We want to make sure to send a full telemetry every now and then even if nothing changed.
    // (There seems to be a problem on the server, not being able to cope with that).
    //
    // This is in addition to the telemetry changes tracker.
    uint32_t last_full_telemetry;
    std::variant<Planner, Registrator> guts;
    Printer &printer;
    SharedBuffer &buffer;

    using ServerResp = std::variant<std::monostate, Command, http::Error>;

    Planner &planner();

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
