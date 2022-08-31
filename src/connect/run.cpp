#include "run.hpp"
#include "marlin_printer.hpp"
#include "connect.hpp"

namespace con {

void run() {
    SharedBuffer buffer;
    MarlinPrinter printer(buffer);
    connect client(printer, buffer);
    client.run();
}

}
