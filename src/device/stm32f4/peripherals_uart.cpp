#include <device/peripherals_uart.hpp>

#include <buddy/main.h>
#include <device/board.h>
#include <device/hal.h>
#include <espif.h>
#include <option/has_mmu2.h>
#include <option/has_puppies.h>
#include <option/has_tmc_uart.h>
#include <printers.h>

#if HAS_PUPPIES()
    #include <puppies/PuppyBus.hpp>
#endif

#if BOARD_IS_BUDDY()
    #define UART_TMC USART2
    #define UART_ESP USART6
#elif BOARD_IS_XBUDDY()
    #if PRINTER_IS_PRUSA_iX()
        #define UART_PUPPIES USART6
    #else
        #define UART_MMU USART6
    #endif
    #define UART_ESP UART8
#elif BOARD_IS_XLBUDDY()
    #define UART_PUPPIES USART3
    #define UART_ESP     UART8
#else
    #error "Unknown printer"
#endif

#if HAS_TMC_UART()
UART_HandleTypeDef uart_handle_for_tmc;
static uint8_t uart_for_tmc_rx_data[32];
buddy::hw::BufferedSerial uart_for_tmc {
    &uart_handle_for_tmc,
    nullptr,
    uart_for_tmc_rx_data,
    sizeof(uart_for_tmc_rx_data),
    buddy::hw::BufferedSerial::CommunicationMode::IT,
};
void uart_init_tmc() {
    uart_handle_for_tmc.Instance = UART_TMC;
    uart_handle_for_tmc.Init.BaudRate = 115'200;
    uart_handle_for_tmc.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle_for_tmc.Init.StopBits = UART_STOPBITS_1;
    uart_handle_for_tmc.Init.Parity = UART_PARITY_NONE;
    uart_handle_for_tmc.Init.Mode = UART_MODE_TX_RX;
    uart_handle_for_tmc.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle_for_tmc.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_HalfDuplex_Init(&uart_handle_for_tmc) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if HAS_PUPPIES()
UART_HandleTypeDef uart_handle_for_puppies;
// Rx data has to absorb any reponse. Standard Modbus response fits in 256 bytes
static uint8_t uart_for_puppies_rx_data[256];
buddy::hw::BufferedSerial uart_for_puppies {
    &uart_handle_for_puppies,
    buddy::puppies::PuppyBus::HalfDuplexCallbackSwitch,
    uart_for_puppies_rx_data,
    sizeof(uart_for_puppies_rx_data),
    buddy::hw::BufferedSerial::CommunicationMode::DMA,
};
void uart_init_puppies() {
    uart_handle_for_puppies.Instance = UART_PUPPIES;
    uart_handle_for_puppies.Init.BaudRate = 230'400;
    uart_handle_for_puppies.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle_for_puppies.Init.StopBits = UART_STOPBITS_1;
    uart_handle_for_puppies.Init.Parity = UART_PARITY_NONE;
    uart_handle_for_puppies.Init.Mode = UART_MODE_TX_RX;
    uart_handle_for_puppies.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle_for_puppies.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart_handle_for_puppies) != HAL_OK) {
        Error_Handler();
    }
}
#endif

#if HAS_MMU2()
UART_HandleTypeDef uart_handle_for_mmu;
static uint8_t uart_for_mmu_rx_data[32];
buddy::hw::BufferedSerial uart_for_mmu {
    &uart_handle_for_mmu,
    nullptr,
    uart_for_mmu_rx_data,
    sizeof(uart_for_mmu_rx_data),
    buddy::hw::BufferedSerial::CommunicationMode::DMA,
};
void uart_init_mmu() {
    uart_handle_for_mmu.Instance = UART_MMU;
    uart_handle_for_mmu.Init.BaudRate = 115'200;
    uart_handle_for_mmu.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle_for_mmu.Init.StopBits = UART_STOPBITS_1;
    uart_handle_for_mmu.Init.Parity = UART_PARITY_NONE;
    uart_handle_for_mmu.Init.Mode = UART_MODE_TX_RX;
    uart_handle_for_mmu.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle_for_mmu.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart_handle_for_mmu) != HAL_OK) {
        Error_Handler();
    }
}
#endif

UART_HandleTypeDef uart_handle_for_esp;
void uart_init_esp() {
    uart_handle_for_esp.Instance = UART_ESP;
    // In tester mode ESP UART is being used to talk to the testing station,
    // so let's just initialize it to the correct baudrate.
    // Eventually, this will be initialized to higher baudrate after ESP flashing
    // if we are not in tester mode.
    uart_handle_for_esp.Init.BaudRate = 115'200;
    uart_handle_for_esp.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle_for_esp.Init.StopBits = UART_STOPBITS_1;
    uart_handle_for_esp.Init.Parity = UART_PARITY_NONE;
    uart_handle_for_esp.Init.Mode = UART_MODE_TX_RX;
    uart_handle_for_esp.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle_for_esp.Init.OverSampling = UART_OVERSAMPLING_8;
    if (HAL_UART_Init(&uart_handle_for_esp) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
#if HAS_TMC_UART()
    if (huart == &uart_handle_for_tmc) {
        uart_for_tmc.WriteFinishedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &uart_handle_for_puppies) {
        uart_for_puppies.WriteFinishedISR();
    }
#endif

#if HAS_MMU2()
    if (huart == &uart_handle_for_mmu) {
        uart_for_mmu.WriteFinishedISR();
    }
#endif

    if (huart == &uart_handle_for_esp) {
        return espif_tx_callback();
    }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
#if HAS_TMC_UART()
    if (huart == &uart_handle_for_tmc) {
        uart_for_tmc.FirstHalfReachedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &uart_handle_for_puppies) {
        uart_for_puppies.FirstHalfReachedISR();
    }
#endif

#if HAS_MMU2()
    if (huart == &uart_handle_for_mmu) {
        uart_for_mmu.FirstHalfReachedISR();
    }
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if HAS_TMC_UART()
    if (huart == &uart_handle_for_tmc) {
        uart_for_tmc.SecondHalfReachedISR();
    }
#endif

#if HAS_PUPPIES()
    if (huart == &uart_handle_for_puppies) {
        uart_for_puppies.SecondHalfReachedISR();
    }
#endif

#if HAS_MMU2()
    if (huart == &uart_handle_for_mmu) {
        uart_for_mmu.SecondHalfReachedISR();
    }
#endif
}
