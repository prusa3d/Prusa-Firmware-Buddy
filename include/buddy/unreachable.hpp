/// @file
#pragma once

#include <common/bsod.h>

/// Declare codepath as unreachable.
///
/// It is a hard error if the codepath is actually taken.
/// This is _not_ std::unreachable() and that is correct.
/// We want the check to be present even in release mode.
///
/// This is a macro. We want to provide some basic debug
/// information in order to distinguisgh BSODs at first
/// glance, without needing to produce full stack trace.
/// Also, we want the different callsites to actually
/// differ because inter-procedural optimization might
/// fold them and prevent setting breakpoints.
#define BUDDY_UNREACHABLE() _bsod("unreachable", __FILE_NAME__, __LINE__)
