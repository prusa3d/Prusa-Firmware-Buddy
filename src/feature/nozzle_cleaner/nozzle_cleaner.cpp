#include "nozzle_cleaner.hpp"

namespace nozzle_cleaner {

const char cleaning_sequence[] = "G1 X222.49 Y303.28 F5000 ;B8\n"
                                 "G1 X240.88 Y284.89 F2000 ;B1\n"
                                 "G1 X243.2 Y296.12  ;B6\n"
                                 "G1 X232.48 Y285.4  ;B2\n"
                                 "G1 X238.46 Y300.86  ;B7\n"
                                 "G1 X227.74 Y290.14  ;B4\n"
                                 "G1 X233.72 Y305.6  ;B9\n"
                                 "G1 X223 Y294.88  ;B5\n"
                                 "G1 X238.46 Y300.86  ;B7\n"
                                 "G1 X227.74 Y290.14  ;B4\n"
                                 "G1 X243.2 Y296.12  ;B6\n"
                                 "G1 X243.71 Y287.72  ;B3\n"
                                 "G1 X225.32 Y306.11  ;B10\n"
                                 "G1 X227.74 Y290.14  ;B4\n"
                                 "G1 X233.72 Y305.6  ;B9\n"
                                 "G1 X240.88 Y284.89  ;B1\n";

} // namespace nozzle_cleaner
