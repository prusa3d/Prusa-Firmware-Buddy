#include "handler.h"

using std::string_view;

namespace nhttp::handler {

void Idle::step(string_view, bool terminated_by_client, uint8_t *, size_t, Step &out) {
    if (terminated_by_client) {
        out = Step { 0, 0, Terminating { 0, Done::Close } };
    } else {
        out = Step { 0, 0, Continue() };
    }
}

void Terminating::step(string_view input, bool client_closed, uint8_t *, size_t, Step &out) {
    if (client_closed) {
        eat_input = 0;
    }
    if (!input.empty() && eat_input) {
        out = Step { input.size(), 0, Continue() };
    } else {
        out = Step { 0, 0, Continue() };
    }
}

} // namespace nhttp::handler
