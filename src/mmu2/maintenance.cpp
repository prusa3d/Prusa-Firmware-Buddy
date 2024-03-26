#include "maintenance.hpp"
#include "fail_bucket.hpp"

#include <config_store/store_instance.hpp>

namespace MMU2 {

namespace {

    constexpr uint32_t maintenance_every = 30000;

}

std::optional<MaintenanceReason> check_maintenance() {
    if (FailLeakyBucket::instance.reached_limit()) {
        return MaintenanceReason::Failures;
    }

    if (config_store().mmu_changes.get() - config_store().mmu_last_maintenance.get() >= maintenance_every) {
        return MaintenanceReason::Changes;
    }

    return std::nullopt;
}

} // namespace MMU2
