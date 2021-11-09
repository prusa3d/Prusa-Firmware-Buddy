/**
 * @file non_file_printing_counter.cpp
 * @author Radek Vana
 * @date 2021-04-29
 */

#include "non_file_printing_counter.hpp"
#include "rtos_api.hpp"

unsigned NonFilePrintingCounter::counter = 0;

void NonFilePrintingCounter::increment() {
    CriticalSection C;
    ++counter;
}

void NonFilePrintingCounter::decrement() {
    CriticalSection C;
    if (counter)
        --counter;
}

bool NonFilePrintingCounter::IsPrinting() {
    CriticalSection C;
    return counter != 0;
}
