#define DO_NOT_CHECK_ATOMIC_LOCK_FREE
#include "hal.hpp"

#include <cyphal_tool_offset_sensor_node.hpp>

#include <FreeRTOS.h>
#include <task.h>
#include <freertos/timing.hpp>
#include <device/peripherals.hpp>
#include <can_driver_fdcan.hpp>
#include <device/hal.h>
#include <o1heap/o1heap.hpp>
#include <cstring>
#include <ldc1612.hpp>
#include <sampling_rate_tracker.hpp>
#include <timing.h>
#include <prusa3d/tool_offset_sensor/Data_1_0.h>
#include <bsod/bsod.h>

LOG_COMPONENT_REF(LDC1612)

// This magical incantation is required for fw_descriptor integration in cmake to work.
[[maybe_unused]] __attribute__((section(".fw_descriptor"), used)) const std::byte fw_descriptor[48] {};

// Forward declaration
extern tool_offset_sensor::cyphal::ToolOffsetSensorNode can_node;

namespace {

constexpr LDC1612::ChannelConfig default_ch_config {
    .rcount = 8192,
    .settlecount = 64,
    .fin_divider = 1,
    .fref_divider = 1,
    .drive_current = 30,
    .offset = 0
};

constexpr LDC1612::DeviceConfig device_config {
    .sleep_mode = false,
    .use_external_clock = true,
    .rp_override_en = false,
    .auto_amp_dis = false,
    .mux_config = {
        .deglitch = LDC1612::DeglitchFilter::MHz_10 },
    .error_config = {
        .report_underrange = true,
        .report_overrange = true,
        .report_watchdog = true,
        .report_amplitude_high = false,
        .report_amplitude_low = false,
        .int_on_underrange = false,
        .int_on_overrange = false,
        .int_on_watchdog = false,
        .int_on_amplitude_high = false,
        .int_on_amplitude_low = false,
        .int_on_zero_count = false,
        .int_on_data_ready = true,
    },
    .ch0 = default_ch_config,
    .ch1 = default_ch_config
};

constexpr size_t max_deltas = prusa3d_tool_offset_sensor_Data_1_0_deltas_ARRAY_CAPACITY_;

// Delta compression state for one channel
struct DeltaCompressor {
    prusa3d_tool_offset_sensor_Data_1_0 msg = {};
    int32_t last_sample = 0;
    bool has_first = false;

    void reset() {
        memset(&msg, 0, sizeof(msg));
        last_sample = 0;
        has_first = false;
    }

    // Feed a raw sample. Returns true when the message is full and ready to send.
    bool feed(int32_t sample, uint32_t freq_16_8) {
        if (!has_first) {
            msg.frequency = freq_16_8;
            msg.sample = sample;
            msg.deltas.count = 0;
            last_sample = sample;
            has_first = true;
            return false;
        }

        int32_t delta = sample - last_sample;
        if (delta < INT16_MIN || delta > INT16_MAX || msg.deltas.count >= max_deltas) {
            // Delta overflow or batch full — message is ready to send.
            // The current sample will be the first of the next message.
            // We intentionally do NOT update msg.frequency here: the frame
            // being flushed should carry the frequency measured at the time
            // of its last successfully added sample, not the one that
            // triggered the overflow. The new frequency will go into the
            // next frame via flush().
            return true;
        }

        msg.frequency = freq_16_8;
        msg.deltas.elements[msg.deltas.count++] = static_cast<int16_t>(delta);
        last_sample = sample;
        return false;
    }

    // Flush a partial frame (if any). Returns true if a message was produced.
    bool flush_partial(uint8_t &seq) {
        if (!has_first) {
            return false;
        }
        msg.sequence = seq++;
        return true;
    }

    // Prepare the message for sending, start fresh with a new sample.
    prusa3d_tool_offset_sensor_Data_1_0 flush(uint8_t &sequence, int32_t next_sample, uint32_t freq_16_8) {
        msg.sequence = sequence++;
        auto result = msg;

        // Start new message with the sample that didn't fit
        reset();
        has_first = true;
        msg.sample = next_sample;
        msg.frequency = freq_16_8;
        last_sample = next_sample;

        return result;
    }
};

bool initialize_ldc(LDC1612 &ldc, bool ch0, bool ch1) {
    hal::ldc1612_set_enabled(true);
    freertos::delay(5);

    if (!ldc.is_device_present()) {
        log_error(LDC1612, "LDC1612 not detected");
        return false;
    }

    ldc.reset();
    if (!ldc.initialize(device_config)) {
        log_error(LDC1612, "LDC1612 init failed");
        return false;
    }

    if (ch0 && ch1) {
        ldc.set_dual_channel_mode();
    } else if (ch0) {
        ldc.set_single_channel_mode(LDC1612::Channel::CH0);
    } else {
        ldc.set_single_channel_mode(LDC1612::Channel::CH1);
    }

    return true;
}

using PublishFn = void (*)(const prusa3d_tool_offset_sensor_Data_1_0 &);

void publish_ch0(const prusa3d_tool_offset_sensor_Data_1_0 &msg) { can_node.publish_data_ch0(msg); }
void publish_ch1(const prusa3d_tool_offset_sensor_Data_1_0 &msg) { can_node.publish_data_ch1(msg); }

void clear_data_ready_semaphore() {
    while (hal::ldc_data_ready.try_acquire_for(0)) {
    }
}

bool has_pending_data(const LDC1612::Status &status, bool ch0, bool ch1) {
    return (ch0 && status.unread_conv_ch0) || (ch1 && status.unread_conv_ch1);
}

struct ChannelState {
    SamplingRateTracker<32> rate { 32 };
    DeltaCompressor delta;
    uint8_t seq = 0;
    PublishFn publish;

    explicit ChannelState(PublishFn publish)
        : publish(publish) {}

    void flush_and_publish() {
        if (delta.flush_partial(seq)) {
            publish(delta.msg);
        }
        delta.reset();
    }

    void reset() {
        rate.reset();
        delta.reset();
        seq = 0;
    }

    bool read_and_publish(LDC1612 &ldc, LDC1612::Channel channel, uint32_t now_us) {
        auto data_opt = ldc.read_channel_data(channel);
        if (!data_opt.has_value()) {
            return false;
        }

        // 28 bit resolution has too large dynamic range to fit into delta compression used during transfer
        // reducing resolution to 24bits keeps needed precision and allows data to be transferred reliably
        rate.record(now_us);
        uint32_t freq = rate.get_frequency_16_8();
        int32_t sample = static_cast<int32_t>(data_opt.value() >> 4);

        if (delta.feed(sample, freq)) {
            publish(delta.flush(seq, sample, freq));
        }
        return true;
    }
};

} // namespace

using SensorState = tool_offset_sensor::cyphal::ToolOffsetSensorNode::SensorState;

static void main_task_code(void *) {
    LDC1612 ldc;
    ChannelState ch0_state(publish_ch0);
    ChannelState ch1_state(publish_ch1);

    bool ldc_powered = false;
    bool prev_ch0 = false;
    bool prev_ch1 = false;
    SensorState sensor_state;

    while (true) {
        auto cfg = can_node.get_config();
        bool ch0 = cfg.ch0_enabled;
        bool ch1 = cfg.ch1_enabled;

        bool config_changed = (ch0 != prev_ch0) || (ch1 != prev_ch1);
        prev_ch0 = ch0;
        prev_ch1 = ch1;

        static uint8_t consecutive_failures = 0;
        if (consecutive_failures >= 5) {
            log_error(LDC1612, "Too many consecutive failures, resetting board");
            freertos::delay(100); // delay to propagate message before reset
            hal::reset();
        }

        // Nothing to do - ensure sensor is off and idle
        if (!ch0 && !ch1) {
            if (ldc_powered) {
                ch0_state.flush_and_publish();
                ch1_state.flush_and_publish();
                hal::ldc1612_set_enabled(false);
                clear_data_ready_semaphore();
                ldc_powered = false;

                sensor_state = {};
                can_node.set_sensor_state(sensor_state);
            }
            freertos::delay(10);
            consecutive_failures = 0;
            continue;
        }

        // (Re)configure the sensor when first powering on or config changed
        if (!ldc_powered || config_changed) {
            if (ldc_powered) {
                ch0_state.flush_and_publish();
                ch1_state.flush_and_publish();
                hal::ldc1612_set_enabled(false);
                clear_data_ready_semaphore();
                freertos::delay(1);
            }

            if (!initialize_ldc(ldc, ch0, ch1)) {
                consecutive_failures++;
                ldc_powered = false;
                sensor_state = {};
                sensor_state.sensor_fault = true;
                can_node.set_sensor_state(sensor_state);
                freertos::delay(10);
                continue;
            }

            ldc_powered = true;
            ch0_state.reset();
            ch1_state.reset();
            clear_data_ready_semaphore();

            sensor_state = {};
            sensor_state.ch0_active = ch0;
            sensor_state.ch1_active = ch1;
            can_node.set_sensor_state(sensor_state);
        }

        // Prefer the DRDY interrupt, but recover by polling status in case
        // INTB stayed asserted and we missed the falling edge.
        if (!hal::ldc_data_ready.try_acquire_for(50)) {
            const auto status = ldc.read_status();
            if (!status.has_value() || !has_pending_data(*status, ch0, ch1)) {
                consecutive_failures++;
                continue;
            }
        }

        while (true) {
            const auto status = ldc.read_status();
            if (!status.has_value() || !has_pending_data(*status, ch0, ch1)) {
                consecutive_failures++;
                break;
            }

            if (status->err_chan0 || status->err_chan1) {
                sensor_state.sensor_errors |= status->error_flags;
                can_node.set_sensor_state(sensor_state);
                consecutive_failures++;
            }

            uint32_t now_us = static_cast<uint32_t>(ticks_us());

            if (ch0 && status->unread_conv_ch0) {
                if (ch0_state.read_and_publish(ldc, LDC1612::Channel::CH0, now_us)) {
                    consecutive_failures = 0;
                } else {
                    consecutive_failures++;
                }
            }
            if (ch1 && status->unread_conv_ch1) {
                if (ch1_state.read_and_publish(ldc, LDC1612::Channel::CH1, now_us)) {
                    consecutive_failures = 0;
                } else {
                    consecutive_failures++;
                }
            }
        }
    }
}

namespace {
constexpr size_t main_task_stack_size = 4096 / sizeof(StackType_t);
alignas(32) StackType_t main_task_stack[main_task_stack_size];
StaticTask_t main_task_control_block;

// CAN node app task, processes requests from the CAN. Implemented in can_node
constexpr size_t node_task_stack_size = 1024 / sizeof(StackType_t);
alignas(32) StackType_t node_task_stack[node_task_stack_size];
StaticTask_t node_task_control_block;

// High-priority task that is woken up when we need to receive/send over CAN and handles the request
constexpr size_t can_task_stack_size = 2048 / sizeof(StackType_t);
alignas(32) StackType_t can_task_stack[can_task_stack_size];
StaticTask_t can_task_control_block;

auto get_uid() {
    tool_offset_sensor::cyphal::ToolOffsetSensorNode::UID uid;
    static constexpr size_t copy_bytes = std::min<size_t>(MCU_UID_SIZE, uid.size());
    std::memcpy(uid.data(), reinterpret_cast<const void *>(UID_BASE), copy_bytes);
    std::memset(uid.data() + MCU_UID_SIZE, 0, uid.size() - copy_bytes);
    return uid;
}

can::FdcanDriver can_driver(hal::peripherals::hfdcan1, hal::enable_bit_rate_switch);

// Heap allocated for canard
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

tool_offset_sensor::cyphal::ToolOffsetSensorNode can_node(get_uid());

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

    [[maybe_unused]] TaskHandle_t main_task_handle = xTaskCreateStatic(
        main_task_code,
        "main_task",
        main_task_stack_size,
        nullptr,
        tskIDLE_PRIORITY + 1,
        main_task_stack,
        &main_task_control_block);

    // Start FreeRTOS scheduler and we are done.
    vTaskStartScheduler();
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] char *pcTaskName) {
    std::abort();
}

extern "C" void vApplicationTickHook(void) {
    HAL_IncTick();
}

[[noreturn]] void __attribute__((noreturn, format(__printf__, 1, 4)))
_bsod(const char *fmt, const char *file_name, int line_number, ...) {
    (void)fmt, (void)file_name, (void)line_number;
    hal::panic();
}

namespace puppy::fault {

bool trigger_fault(tool_offset_sensor::cyphal::Fault fault) {
    return can_node.set_fault(fault);
}

bool trigger_fault(puppy::fault::SharedFault fault) {
    return trigger_fault(tool_offset_sensor::cyphal::from_shared(fault));
}

} // namespace puppy::fault
