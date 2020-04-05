// can.h
//basic api for single can
//single filter instance
//single tx instance
//to be extended

//to enable can define HAL_CAN_MODULE_ENABLED
//in stm32f4xx_hal_conf
//CAN2 share pins with uart1 and WP2_pin
#pragma once
#include "stm32f4xx_hal.h"
#ifdef HAL_CAN_MODULE_ENABLED

    #include <stdint.h>
    #include <stdlib.h> //size_t

    #ifdef __cplusplus
extern "C" {
    #endif //__cplusplus

typedef struct {
    uint32_t is_29_ID : 1; //1 == 29 bit ID, 0 == 11 bit
    uint32_t ID : 29;
    //uncomment DLC if needed, but i think it is not
    //uint32_t DLC : 4; //data 0 - 8B
} CAN_HEADER_t;

typedef struct {
    CAN_HEADER_t header;
    uint8_t data[8];
} CAN_MSG_t;

extern int CAN2_Init();
extern int CAN2_Start();
extern void CAN2_Stop();
extern int CAN2_is_initialized();
extern void CAN2_Tx(uint8_t *data, size_t sz);
extern void CAN2_Tx8(uint8_t *data_8byte);
extern int CAN2_try_Rx(CAN_MSG_t *msg);

//TX setting
extern int CAN2_set_tx_DLC(uint32_t DLC);
extern void CAN2_set_tx_RTR_data();
extern void CAN2_set_tx_RTR_remote();
extern int CAN2_set_tx_StdId(uint32_t id);
extern int CAN2_set_tx_ExtId(uint32_t id);

//RX setting
void CAN2_set_rx_filter_MASK32_STD(uint32_t mask);             //1 32bit filter mask for 11bit ID
void CAN2_set_rx_filter_MASK32_EXT(uint32_t mask);             //1 32bit filter mask for 29bit ID
void CAN2_set_rx_filter_MASK16(uint16_t first, uint16_t next); //2 16bit filter mask
void CAN2_set_rx_filter_LIST32_STD(uint32_t list);             //1 32bit filter list for 11bit ID
void CAN2_set_rx_filter_LIST32_EXT(uint32_t list);             //1 32bit filter list for 29bit ID
void CAN2_set_rx_filter_LIST16(uint16_t first, uint16_t next); //2 16bit filter list
void CAN2_set_rx_filter_fifo0();
void CAN2_set_rx_filter_fifo1();
int CAN2_set_rx_filter_bank(uint32_t bank); //single instance max 13
void CAN2_set_rx_filter_activate();
void CAN2_set_rx_filter_deactivate();

void CAN2_TX_IRQHandler(void);
void CAN2_RX0_IRQHandler(void);
void CAN2_RX1_IRQHandler(void);
void CAN2_SCE_IRQHandler(void);
    #ifdef __cplusplus
}
    #endif //__cplusplus

#endif //HAL_CAN_MODULE_ENABLED
