#pragma once
#include <optional>
#include "resources/revision.hpp"

namespace buddy::resources {

bool is_bootstrap_needed();

using ProgressHook = void (*)(int percent_done, std::optional<const char *> description);

bool bootstrap(ProgressHook progress_hook);

};
