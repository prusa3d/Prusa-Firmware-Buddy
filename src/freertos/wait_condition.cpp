#include <freertos/wait_condition.hpp>

using namespace freertos;

void WaitCondition::notify_one() {
    const int count = waiter_count.fetch_sub(1);
    // If there was no one waiting, increase the one back again, because we're not releasing anything
    if (count <= 0) {
        waiter_count++;
        return;
    }
    semaphore.release_blocking();
}
void freertos::WaitCondition::notify_all() {
    // Fetch waiter_count and set it to zero
    int count = waiter_count.exchange(0);

    // If it was below zero, no tasks were locked there
    // and other tasks will be atomically increasing by that amount in notify_one, we need to account for that
    if (count < 0) {
        waiter_count += count;
        return;
    }

    while (count--) {
        semaphore.release_blocking();
    }
}
