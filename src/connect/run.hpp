#pragma once

namespace connect_client {

/// The "main" loop for connect.
///
/// Run in a dedicated thread.
///
/// Shall run even in case connect is disabled (it'll just "sleep" most of the
/// time and will simply pick it up once connect is enabled). Never terminates.
void run() __attribute__((noreturn));

} // namespace connect_client
