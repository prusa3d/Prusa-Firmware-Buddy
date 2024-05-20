#pragma once

#include <guiconfig/guiconfig.h>

namespace dialog_text_input {

#if HAS_LARGE_DISPLAY() // Mk4, XL, ...
static constexpr int button_cols = 6;
static constexpr int button_rows = 3;

#else // Mini
static constexpr int button_cols = 4;
static constexpr int button_rows = 5;

#endif

static constexpr int button_count = button_cols * button_rows;

} // namespace dialog_text_input
