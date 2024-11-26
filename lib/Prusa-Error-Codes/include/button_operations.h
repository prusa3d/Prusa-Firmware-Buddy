#pragma once	
#include <stdint.h>	

/// Will be mapped onto dialog button responses in the FW	
/// Those responses have their unique+translated texts as well	
enum class ButtonOperations : uint8_t {
    NoOperation = 0,	
    Retry = 1,	
    Continue = 2,	
    ResetMMU = 3,	
    Unload = 4,	
    StopPrint = 5,	
    DisableMMU = 6,	
    Skip = 7,
};
