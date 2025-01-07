#include "nozzle_cleaner.hpp"

namespace nozzle_cleaner {

ConstexprString load_sequence = "G1 X267.4 Y284.75 F3000\n"
                                "G1 X253.4 Y284.75 F3000\n"
                                "G1 X267.4 Y284.75 F3000\n"
                                "G1 X253.4 Y284.75 F3000\n"
                                "G1 X222.49 Y303.28 F5000\n"
                                "G1 X240.88 Y284.89 F2000\n"
                                "G1 X243.2 Y296.12\n"
                                "G1 X232.48 Y285.4\n"
                                "G1 X238.46 Y300.86\n"
                                "G1 X227.74 Y290.14\n"
                                "G1 X233.72 Y305.6\n"
                                "G1 X223 Y294.88\n"
                                "G1 X238.46 Y300.86\n"
                                "G1 X227.74 Y290.14\n"
                                "G1 X243.2 Y296.12\n"
                                "G1 X243.71 Y287.72\n"
                                "G1 X225.32 Y306.11\n"
                                "G1 X227.74 Y290.14\n"
                                "G1 X233.72 Y305.6\n"
                                "G1 X240.88 Y284.89\n"
                                "G27";

ConstexprString unload_sequence = "G1 X267.4 Y284.75 F3000\n"
                                  "G1 X253.4 Y284.75 F3000\n"
                                  "G1 X267.4 Y284.75 F3000\n"
                                  "G1 X253.4 Y284.75 F3000\n"
                                  "G27";

ConstexprString runout_sequence = "G1 X267.4 Y284.75 F3000\n"
                                  "G1 X253.4 Y284.75 F3000\n"
                                  "G1 X267.4 Y284.75 F3000\n"
                                  "G1 X253.4 Y284.75 F3000\n"
                                  "G1 X222.49 Y303.28 F5000\n"
                                  "G1 X240.88 Y284.89 F2000\n"
                                  "G1 X243.2 Y296.12\n"
                                  "G1 X232.48 Y285.4\n"
                                  "G1 X238.46 Y300.86\n"
                                  "G1 X227.74 Y290.14\n"
                                  "G1 X233.72 Y305.6\n"
                                  "G1 X223 Y294.88\n"
                                  "G1 X238.46 Y300.86\n"
                                  "G1 X227.74 Y290.14\n"
                                  "G1 X243.2 Y296.12\n"
                                  "G1 X243.71 Y287.72\n"
                                  "G1 X225.32 Y306.11\n"
                                  "G1 X227.74 Y290.14\n"
                                  "G1 X233.72 Y305.6\n"
                                  "G1 X240.88 Y284.89";

ConstexprString g12_sequence = runout_sequence;

ConstexprString load_filename = "nozzle_cleaner_load";
ConstexprString unload_filename = "nozzle_cleaner_unload";
ConstexprString runout_filename = "nozzle_cleaner_runout";
ConstexprString g12_filename = "nozzle_cleaner_g12";

} // namespace nozzle_cleaner
