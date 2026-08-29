#pragma once

// Stub for the generated option header. This is what every other printer builds
// with -- MK4, MK3.5, XL, MINI and the Core Ones -- so sntp_client_step() looks
// at both interfaces. Note the critical-infrastructure editions of the XL and
// the Core One ship this same firmware with the wifi hardware physically
// removed, which is a wifi interface that never comes up rather than a
// different build.
#define HAS_ESP() 1
