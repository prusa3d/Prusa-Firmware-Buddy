/// @file panic.h
#pragma once
#include <stdint.h>
#include "logic/error_codes.h"

/// Switches the currently active logic state machine into an error state of code ec.
/// It shall be used to halt the firmware while retaining the capability of reporting the error state to the printer
/// - a kind of similar to runtime assertions.
/// Implementation is in main.cpp, where we know the currently active logic state machine.
/// The only way out is to reset the board.
extern void Panic(ErrorCode ec);
