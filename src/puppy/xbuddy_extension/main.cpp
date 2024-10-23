#include "hal.hpp"

extern "C" int main() {
    hal::init();
    hal::panic();
}
