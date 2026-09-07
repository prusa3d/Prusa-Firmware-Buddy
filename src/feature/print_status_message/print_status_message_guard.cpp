#include "print_status_message_guard.hpp"
#include "print_status_message_mgr.hpp"

#include <marlin_server.hpp>
#include <bsod/bsod.h>

PrintStatusMessageGuard::PrintStatusMessageGuard(bool clear_temporary_msg) {
    debug_assert(marlin_server::is_marlin_server_thread());

    auto &psm = print_status_message();
    std::scoped_lock guard(psm.mutex_);

    parent_guard_ = psm.active_guard_;
    record_.id = psm.id_counter_++;

    if (clear_temporary_msg) {
        psm.temporary_message_ = {};
    }

    psm.active_guard_ = this;
}

PrintStatusMessageGuard::~PrintStatusMessageGuard() {
    auto &psm = print_status_message();
    std::scoped_lock guard(psm.mutex_);

    debug_assert(psm.active_guard_ == this);
    psm.active_guard_ = parent_guard_;
}

void PrintStatusMessageGuard::update(const Message &msg) {
    auto &psm = print_status_message();
    std::scoped_lock guard(psm.mutex_);

    record_.message = msg;
    psm.add_history_item_nolock(record_);
}
