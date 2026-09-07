#include "main.hpp"
#include "hal.hpp"
#include "nfc.hpp"

#include <FreeRTOS.h>
#include <task.h>
#include <freertos/timing.hpp>
#include <device/peripherals.hpp>
#include <cyphal_anfc_node.hpp>
#include <option/can_bus_type.h>
#if CAN_BUS_TYPE_IS_UART()
    #include <can_driver_uart.hpp>
#else
    #include <can_driver_fdcan.hpp>
#endif
#include <device/hal.h>
#include <o1heap/o1heap.hpp>
#include <openprinttag_nfcv/opt_nfcv.hpp>
#include <option/nfc_st25r3919b_enabled_antennas.h>
#include <bsod/bsod.h>

// This magical incantation is required for fw_descriptor integration in cmake to work.
[[maybe_unused]] __attribute__((section(".fw_descriptor"), used)) const std::byte fw_descriptor[48] {};

// Lowering CANARD_NODE_ID_MAX reduces RAM requirements. It is a change in the canard.h library -> make sure we don't accidentally remove it when we update
static_assert(CANARD_NODE_ID_MAX == 15);

LOG_COMPONENT_DEF(nfc);

namespace {

// CAN node app task, processes requests from the CAN. Implemented in can_node
constexpr const size_t node_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t node_task_stack[node_task_stack_size];
StaticTask_t node_task_control_block;

// High-priority task that is woken up when we need to receive/send over CAN and handles the request
constexpr const size_t can_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t can_task_stack[can_task_stack_size];
StaticTask_t can_task_control_block;

// NFC reader task, handles lenghty blocking interactions with the NFC tags. Implemented in nfc_task
constexpr const size_t nfc_task_stack_size = 2240 / sizeof(StackType_t);
alignas(32) StackType_t nfc_task_stack[nfc_task_stack_size];
StaticTask_t nfc_task_control_block;

auto get_uid() {
    anfc::cyphal::ANFCNode::UID uid;
    static constexpr size_t copy_bytes = std::min<size_t>(MCU_UID_SIZE, uid.size());
    memcpy(uid.data(), reinterpret_cast<const void *>(UID_BASE), copy_bytes);
    memset(uid.data() + MCU_UID_SIZE, 0, uid.size() - copy_bytes);
    return uid;
}

#if CAN_BUS_TYPE_IS_UART()
can::UartDriver can_driver(
    #ifdef STM32C0
    hal::peripherals::huart1
    #elif STM32H5
    // #error dead code found by automatic analyses (see BFW-5461)
    hal::peripherals::huart2
    #else
        #error
    #endif
);
#else
can::FdcanDriver can_driver(hal::peripherals::hfdcan1, hal::enable_bit_rate_switch);
#endif

/// Heap allocated for canard
O1Heap<8192> canard_heap;

void *canard_heap_allocate(CanardInstance *, size_t bytes) {
    return canard_heap.alloc(bytes);
}
void canard_heap_free(CanardInstance *, void *ptr) {
    canard_heap.free(ptr);
}
} // namespace

// Cannot be in private namespace - linked with src/can
// Also, has to be initialized before can_node
can::cyphal::Task::RxQueue<1> cyphal_task_rx_queue;
can::cyphal::Task can::cyphal::cyphal_task(can_driver, cyphal_task_rx_queue, 32, &canard_heap_allocate, &canard_heap_free);

anfc::cyphal::ANFCNode can_node(get_uid());

static constexpr openprinttag::ReaderAntenna convert_antenna(const option::NfcSt25r3919bEnabledAntennas enabled_antennas) {
    switch (enabled_antennas) {
    case option::NfcSt25r3919bEnabledAntennas::antenna_1:
        return openprinttag::ReaderAntenna { 0 };
    case option::NfcSt25r3919bEnabledAntennas::antenna_2:
        return openprinttag::ReaderAntenna { 1 };
    case option::NfcSt25r3919bEnabledAntennas::both:
        return openprinttag::OPTBackend::no_antenna_enforce;
    }
}

static constexpr openprinttag::OPTBackend::Config ll_config {
    .enforced_antenna = convert_antenna(option::nfc_st25r3919b_enabled_antennas),
};

// !!! Buddy implementation cannot correctly track multiple tags per antenna, the implementation will need to be adjusted if this value changes
static_assert(ll_config.max_known_tags_per_antenna == 1);

static openprinttag::OPTBackend_NFCV ll_reader { nfc::reader_1, ll_config };

NFCTask nfc_task(
    ll_reader, [](prusa3d_nfc_event_Event_1_0 &event) { can_node.enqueue_event(event); }, nfc::reconfigure_readers);

extern "C" int main() {
    hal::init();

    // Set up Cyphal node and communication basics
    can_node.init();

    [[maybe_unused]] TaskHandle_t can_task_handle = xTaskCreateStatic(
        can::cyphal::Task::task,
        "can_task",
        can_task_stack_size,
        &can::cyphal::cyphal_task,
        tskIDLE_PRIORITY + 2,
        can_task_stack,
        &can_task_control_block);

    [[maybe_unused]] TaskHandle_t node_task_handle = xTaskCreateStatic(
        [](void *) { can_node.task(); },
        "node_task",
        node_task_stack_size,
        nullptr,
        tskIDLE_PRIORITY + 1,
        node_task_stack,
        &node_task_control_block);

    [[maybe_unused]] TaskHandle_t nfc_task_handle = xTaskCreateStatic(
        [](void *) {
            nfc::readers_init();
            nfc_task.task();
        },
        "nfc_task",
        nfc_task_stack_size,
        nullptr,
        tskIDLE_PRIORITY + 1,
        nfc_task_stack,
        &nfc_task_control_block);

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] char *pcTaskName) {
    std::abort();
}

extern "C" void vApplicationTickHook(void) {
    // Used by HAL_SPI
    HAL_IncTick();
}

[[noreturn]] void __attribute__((noreturn, format(__printf__, 1, 4)))
_bsod(const char *fmt, const char *file_name, int line_number, ...) {
    (void)fmt, (void)file_name, (void)line_number;
    hal::panic();
}

namespace puppy::fault {

bool trigger_fault(anfc::cyphal::Fault fault) {
    auto ret = can_node.set_fault(fault);

    return ret;
}

bool trigger_fault(puppy::fault::SharedFault fault) {
    return trigger_fault(anfc::cyphal::from_shared(fault));
}

} // namespace puppy::fault
