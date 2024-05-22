/// @file debouncer.cpp
#include "debouncer.h"

namespace modules {
namespace debounce {

// original idea from: https://www.eeweb.com/debouncing-push-buttons-using-a-state-machine-approach
void Debouncer::Step(uint16_t time, bool press) {
    switch (f.state) {
    case State::Waiting:
        if (press) {
            f.state = State::Detected;
            timeLastChange = time;
            f.tmp = press;
        }
        break;
    case State::Detected:
        if (f.tmp == press) {
            if (time - timeLastChange >= debounceTimeout) {
                f.state = State::WaitForRelease;
            }
        } else {
            f.state = State::Waiting;
        }
        break;
    case State::WaitForRelease:
        if (!press) {
            f.state = State::Update;
        }
        break;
    case State::Update:
        f.state = State::Waiting;
        timeLastChange = time;
        f.tmp = false;
        break;
    default:
        f.state = State::Waiting;
        timeLastChange = time;
        f.tmp = false;
    }
}

} // namespace debounce
} // namespace modules
