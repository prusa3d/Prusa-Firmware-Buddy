#include "run.hpp"
#include "marlin_printer.hpp"
#include "connect.hpp"

namespace connect {

void run() {
    SharedBuffer buffer;
    MarlinPrinter printer(buffer);
    connect client(printer, buffer);
    client.run();
}

}
