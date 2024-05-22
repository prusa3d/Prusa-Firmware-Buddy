#pragma once
#include <optional>
#include <functional>

#include "resources/revision.hpp"

namespace buddy::resources {

bool has_resources(const Revision &revision);

enum class BootstrapStage {
    LookingForBbf,
    PreparingBootstrap,
    CopyingFiles,
};

using ProgressHook = std::function<void(int percent_done, BootstrapStage stage)>;

bool bootstrap(const Revision &revision, ProgressHook progress_hook);

}; // namespace buddy::resources
