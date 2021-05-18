/**
 * @file gui_time.hpp
 * @author Radek Vana
 * @brief wrapped acces to tick functions
 * @date 2021-04-21
 */

#include <cstdint>

namespace gui {
void TickLoop();                       // call this function in loop
uint32_t GetTick();                    // current loop tick value, every call in current loop returns same value
uint64_t GetTickU64();                 // current loop tick value, this will never overflow
uint32_t GetTick_IgnoreTickLoop();     // this should be used rarely
uint32_t GetTick_ForceActualization(); //needed during gui start
};
