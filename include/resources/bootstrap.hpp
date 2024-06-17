#pragma once
#include <optional>
#include <inplace_function.hpp>

#include "resources/revision.hpp"

namespace buddy::resources {

bool has_resources(const Revision &revision);

enum class BootstrapStage {
    LookingForBbf,
    PreparingBootstrap,
    CopyingFiles,
};

using ProgressHook = stdext::inplace_function<void(int percent_done, BootstrapStage stage)>;

bool bootstrap(const Revision &revision, ProgressHook progress_hook);

}; // namespace buddy::resources
