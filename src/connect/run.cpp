#include "run.hpp"
#include "marlin_printer.hpp"
#include "error_printer.hpp"
#include "connect.hpp"

namespace connect_client {

void run() {
    SharedBuffer buffer;
    MarlinPrinter printer;
    Connect client(printer, buffer);
    client.run();
}

void run_error() {
    SharedBuffer buffer; // TODO: Can we get rid of this one?
    // Dynamic allocation to shrink ErrorPrinter's size to avoid stack overflows.
    // Memory fragmentation is not an issue here, we are just showing error screen
    // and reporting it to the Prusa Connect until next reset.
    // Null check is deliberately missing, our malloc is not fallable.
    // If this static assertion starts failing, it is a good time to revert this fix.
    static_assert(sizeof(ErrorPrinter) > sizeof(MarlinPrinter));
    auto printer = std::make_unique<ErrorPrinter>();
    Connect client(*printer.get(), buffer);
    client.run();
}

} // namespace connect_client
