#pragma once
#include <stdint.h>
#include <string>

namespace modules {
namespace serial {

using SerialBuff = std::string;

extern SerialBuff tx;
extern SerialBuff rx;

void ClearRX();
void ClearTX();

} // namespace serial
} // namespace modules
