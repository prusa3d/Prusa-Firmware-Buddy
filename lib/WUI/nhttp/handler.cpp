#include "handler.h"

using std::string_view;

namespace nhttp::handler {

Step Idle::step(string_view, bool terminated_by_client, uint8_t *, size_t) {
    if (terminated_by_client) {
        return { 0, 0, Terminating { Done::Close } };
    } else {
        return { 0, 0, Continue() };
    }
}

Step Terminating::step(string_view, bool, uint8_t *, size_t) {
    return { 0, 0, Continue() };
}

Selector::~Selector() {}

}
