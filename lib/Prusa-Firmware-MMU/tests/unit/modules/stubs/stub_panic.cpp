#include "../panic.h"

// For retrival during tests
ErrorCode panic_code = ErrorCode::RUNNING;

void Panic(ErrorCode ec) {
    panic_code = ec;
}
