#include "run.hpp"
#include "marlin_printer.hpp"
#include "connect.hpp"

namespace connect_client {

void run() {
    SharedBuffer buffer;
    MarlinPrinter printer;
    Connect client(printer, buffer);
    client.run();
}

} // namespace connect_client
