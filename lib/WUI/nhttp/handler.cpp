#include "handler.h"

using std::string_view;

namespace nhttp::handler {

Step Idle::step(string_view, bool terminated_by_client, uint8_t *, size_t) {
    if (terminated_by_client) {
        return { 0, 0, Terminating { 0, Done::Close } };
    } else {
        return { 0, 0, Continue() };
    }
}

Step Terminating::step(string_view input, bool client_closed, uint8_t *, size_t) {
    if (client_closed) {
        eat_input = 0;
    }
    if (!input.empty() && eat_input) {
        return { input.size(), 0, Continue() };
    } else {
        return { 0, 0, Continue() };
    }
}

} // namespace nhttp::handler
