/**
 * @file non_file_printing_counter.hpp
 * @author Radek Vana
 * @brief
 * @date 2021-04-29
 */

#include <cstdint>

class NonFilePrintingCounter {
    static unsigned counter;
    static void increment();
    static void decrement();

public:
    NonFilePrintingCounter() { increment(); }
    ~NonFilePrintingCounter() { decrement(); }
    static bool IsPrinting();
};
