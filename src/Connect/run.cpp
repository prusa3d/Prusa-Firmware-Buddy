#include "run.hpp"
#include "marlin_printer.hpp"
#include "connect.hpp"

namespace con {

void run() {
    MarlinPriter printer;
    connect client(printer);
    client.run();
}

}
