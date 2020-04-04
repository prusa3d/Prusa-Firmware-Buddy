// can.h
//basic api for single can
//single filter instance
//single tx instance
//to be extended

#pragma once
#include <stdint.h>
#include <stdlib.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
extern int CAN2_Init();
extern int CAN2_Start();
extern void CAN2_Stop();
extern int CAN2_is_initialized();
extern void CAN2_Tx(uint8_t *data, size_t sz);
extern void CAN2_Tx8(uint8_t *data_8byte);
extern int CAN2_try_Rx(uint8_t *data);

//TX setting
extern int CAN2_set_tx_DLC(uint32_t DLC);
extern void CAN2_set_tx_RTR_data();
extern void CAN2_set_tx_RTR_remote();
extern int CAN2_set_tx_StdId(uint32_t id);
extern int CAN2_set_tx_ExtId(uint32_t id);

//RX setting
void CAN2_set_rx_filter_MASK32(uint32_t mask);                 //1 32bit filter mask
void CAN2_set_rx_filter_MASK16(uint16_t first, uint16_t next); //2 16bit filter mask
void CAN2_set_rx_filter_LIST32(uint32_t list);                 //1 32bit filter list
void CAN2_set_rx_filter_LIST16(uint16_t first, uint16_t next); //2 16bit filter list
void CAN2_set_rx_filter_fifo0();
void CAN2_set_rx_filter_fifo1();
int CAN2_set_rx_filter_bank(uint32_t bank); //single instance max 13
void CAN2_set_rx_filter_activate();
void CAN2_set_rx_filter_deactivate();

#ifdef __cplusplus
}
#endif //__cplusplus
