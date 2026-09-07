///@file
#include "hal_pub.hpp"

#include <stm32h5xx_hal.h>
#include <utils/byte_utils.hpp>

// CAN FD configuration

// Cyphal/CAN Physical Layer Specification v1.0 recommends several standard
// bitrates. One of them is 125k/500k with sampling at 87.5% which we use.

// Let's use the safe 125k bitrate for both arbitration and data.
// In the future, we may try to dynamically enable bitrate switching
// and count errors to see if the bus topology permits faster bitrate.
static constexpr const bool enable_bit_rate_switch = false;

// We are using 48MHz clock as an input.
static constexpr const uint32_t clock_source = RCC_FDCANCLKSOURCE_PLL1Q;
static constexpr const uint32_t clock_frequency = 48'000'000;

static constexpr const uint32_t nominal_bitrate = 125'000;
static constexpr const uint32_t nominal_prescaler = 3;
static constexpr const uint32_t nominal_sync_jump_width = 16;
static constexpr const uint32_t nominal_time_seg_1 = 111;
static constexpr const uint32_t nominal_time_seg_2 = 16;

static constexpr const uint32_t data_bitrate = 500'000;
static constexpr const uint32_t data_prescaler = 3;
static constexpr const uint32_t data_sync_jump_width = 4;
static constexpr const uint32_t data_time_seg_1 = 27;
static constexpr const uint32_t data_time_seg_2 = 4;

// Time quanta must add up.
static_assert(nominal_prescaler * (1 + nominal_time_seg_1 + nominal_time_seg_2) * nominal_bitrate == clock_frequency);
static_assert(data_prescaler * (1 + data_time_seg_1 + data_time_seg_2) * data_bitrate == clock_frequency);

// Ensure all parameters are in HAL range.
static_assert(IS_FDCAN_NOMINAL_PRESCALER(nominal_prescaler));
static_assert(IS_FDCAN_NOMINAL_SJW(nominal_sync_jump_width));
static_assert(IS_FDCAN_NOMINAL_TSEG1(nominal_time_seg_1));
static_assert(IS_FDCAN_NOMINAL_TSEG2(nominal_time_seg_2));
static_assert(IS_FDCAN_DATA_PRESCALER(data_prescaler));
static_assert(IS_FDCAN_DATA_SJW(data_sync_jump_width));
static_assert(IS_FDCAN_DATA_TSEG1(data_time_seg_1));
static_assert(IS_FDCAN_DATA_TSEG2(data_time_seg_2));

// CiA 601-3 recommendation 1: choose prescaler as small as possible.
// This can't be easily (statically) asserted. Computation goes as follows:
// Prescaler is now 3; next smaller value is 2, which gives time quanta count
// clock_frequency/data_bitrate/data_prescaler = 48
// This would put data_time_seg_1 at 48*7/8 = 42 if we want to preserve sampling
// point at 87.5% which is outside the HAL range.
static_assert(data_prescaler == 3);

// CiA 601-3 recommendation 2: choose nominal_sync_jump_width as large as possible.
static_assert(nominal_sync_jump_width == nominal_time_seg_2);

// Cia 601-3 recommendation 3: choose the highest available CAN clock frequency.
// Maybe we could configure other PLL, but for now, 48MHz seems good enough.

// CiA 601-3 recommendation 4: set nominal_prescaler == data_prescaler.
static_assert(nominal_prescaler == data_prescaler);

// CiA 601-3 recommendation 5: configure all CAN nodes to have the same sampling point.
// We can't vouch for all the nodes, but let's assert standard sampling point at 87.5% = 7/8
static_assert(7 * nominal_time_seg_2 == 1 + nominal_time_seg_1);
static_assert(7 * data_time_seg_2 == 1 + data_time_seg_1);

// CiA 601-3 recommendation 6: choose data_sync_jump_width as large as possible.
static_assert(data_sync_jump_width == data_time_seg_2);

// CiA 601-3 recommendation 7: enable TDC for data bit rates ≥ 1 Mbit/s.
static_assert(data_bitrate < 1'000'000);

FDCAN_HandleTypeDef hfdcan;

extern "C" void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *) {
    // enable peripheral clocks
    __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PLL1Q);
    __HAL_RCC_FDCAN_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    {
        // PB5 = FDCAN1RX
        // PB6 = FDCAN1TX
        constexpr GPIO_InitTypeDef config = {
            .Pin = GPIO_PIN_5 | GPIO_PIN_6,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = enable_bit_rate_switch ? GPIO_SPEED_FREQ_VERY_HIGH : GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF9_FDCAN1,
        };
        HAL_GPIO_Init(GPIOB, &config);
    }
    {
        // PB3 = NFAULT
        constexpr GPIO_InitTypeDef config {
            .Pin = GPIO_PIN_3,
            .Mode = GPIO_MODE_INPUT,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOB, &config);
    }
    __HAL_RCC_GPIOC_CLK_ENABLE();
    {
        // PC15 = ENABLE
        constexpr GPIO_InitTypeDef config {
            .Pin = GPIO_PIN_15,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOC, &config);
    }

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
}

extern "C" void FDCAN1_IT0_IRQHandler() {
    HAL_FDCAN_IRQHandler(&hfdcan);
}

static void fdcan_init() {
    hfdcan.Instance = FDCAN1;
    hfdcan.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan.Init.FrameFormat = enable_bit_rate_switch ? FDCAN_FRAME_FD_BRS : FDCAN_FRAME_FD_NO_BRS;
    hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission = ENABLE;
    hfdcan.Init.TransmitPause = DISABLE;
    hfdcan.Init.ProtocolException = DISABLE;
    hfdcan.Init.NominalPrescaler = nominal_prescaler;
    hfdcan.Init.NominalSyncJumpWidth = nominal_sync_jump_width;
    hfdcan.Init.NominalTimeSeg1 = nominal_time_seg_1;
    hfdcan.Init.NominalTimeSeg2 = nominal_time_seg_2;
    hfdcan.Init.DataPrescaler = data_prescaler;
    hfdcan.Init.DataSyncJumpWidth = data_sync_jump_width;
    hfdcan.Init.DataTimeSeg1 = data_time_seg_1;
    hfdcan.Init.DataTimeSeg2 = data_time_seg_2;
    hfdcan.Init.StdFiltersNbr = 0;
    hfdcan.Init.ExtFiltersNbr = 0;
    hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan) != HAL_OK) {
        abort();
    }
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan,
            FDCAN_REJECT,
            FDCAN_ACCEPT_IN_RX_FIFO0,
            FDCAN_REJECT_REMOTE,
            FDCAN_REJECT_REMOTE)
        != HAL_OK) {
        abort();
    }
}

static uint32_t size_to_dlc(size_t size) {
    if (size <= 0) {
        return FDCAN_DLC_BYTES_0;
    }
    if (size <= 1) {
        return FDCAN_DLC_BYTES_1;
    }
    if (size <= 2) {
        return FDCAN_DLC_BYTES_2;
    }
    if (size <= 3) {
        return FDCAN_DLC_BYTES_3;
    }
    if (size <= 4) {
        return FDCAN_DLC_BYTES_4;
    }
    if (size <= 5) {
        return FDCAN_DLC_BYTES_5;
    }
    if (size <= 6) {
        return FDCAN_DLC_BYTES_6;
    }
    if (size <= 7) {
        return FDCAN_DLC_BYTES_7;
    }
    if (size <= 8) {
        return FDCAN_DLC_BYTES_8;
    }
    if (size <= 12) {
        return FDCAN_DLC_BYTES_12;
    }
    if (size <= 16) {
        return FDCAN_DLC_BYTES_16;
    }
    if (size <= 20) {
        return FDCAN_DLC_BYTES_20;
    }
    if (size <= 24) {
        return FDCAN_DLC_BYTES_24;
    }
    if (size <= 32) {
        return FDCAN_DLC_BYTES_32;
    }
    if (size <= 48) {
        return FDCAN_DLC_BYTES_48;
    }
    if (size <= 64) {
        return FDCAN_DLC_BYTES_64;
    }
    abort();
}

static size_t dlc_to_size(uint32_t dlc) {
    switch (dlc) {
    case FDCAN_DLC_BYTES_0:
        return 0;
    case FDCAN_DLC_BYTES_1:
        return 1;
    case FDCAN_DLC_BYTES_2:
        return 2;
    case FDCAN_DLC_BYTES_3:
        return 3;
    case FDCAN_DLC_BYTES_4:
        return 4;
    case FDCAN_DLC_BYTES_5:
        return 5;
    case FDCAN_DLC_BYTES_6:
        return 6;
    case FDCAN_DLC_BYTES_7:
        return 7;
    case FDCAN_DLC_BYTES_8:
        return 8;
    case FDCAN_DLC_BYTES_12:
        return 12;
    case FDCAN_DLC_BYTES_16:
        return 16;
    case FDCAN_DLC_BYTES_20:
        return 20;
    case FDCAN_DLC_BYTES_24:
        return 24;
    case FDCAN_DLC_BYTES_32:
        return 32;
    case FDCAN_DLC_BYTES_48:
        return 48;
    case FDCAN_DLC_BYTES_64:
        return 64;
    }
    abort();
}

void hal::pub::init() {
    fdcan_init();
    HAL_FDCAN_Start(&hfdcan);
    enable_pin_set(true);
}

bool hal::pub::transmit(Identifier identifier, Bytes span) {
    const FDCAN_TxHeaderTypeDef header {
        .Identifier = identifier,
        .IdType = FDCAN_EXTENDED_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = size_to_dlc(span.size()),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = enable_bit_rate_switch ? FDCAN_BRS_ON : FDCAN_BRS_OFF,
        .FDFormat = FDCAN_FD_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0,
    };
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &header, (const uint8_t *)span.data()) == HAL_OK;
}

bool hal::pub::receive(RxFrame &frame) {
    FDCAN_RxHeaderTypeDef header;
    if (HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &header, frame.data) == HAL_OK) {
        frame.identifier = header.Identifier;
        frame.size = dlc_to_size(header.DataLength);
        return true;
    } else {
        return false;
    }
}

void hal::pub::enable_pin_set(bool b) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, b ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool hal::pub::nfault_pin_get() {
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET;
}
