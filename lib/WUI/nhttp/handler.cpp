#include "handler.h"

using std::string_view;

namespace nhttp::handler {

Step Idle::step(string_view, uint8_t *, size_t) {
    return { 0, 0, Continue() };
}

Step Terminating::step(string_view, uint8_t *, size_t) {
    return { 0, 0, Continue() };
}

Selector::~Selector() {}

}
