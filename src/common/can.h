// can.h
#pragma once
#include <stdint.h>
#include <stdlib.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
extern int CAN2_Init();
extern int CAN2_Start();
extern int CAN2_is_initialized();
extern void CAN2_Tx(uint8_t *data, size_t sz);
extern void CAN2_Tx8(uint8_t *data_8byte);
extern int CAN2_try_Rx(uint8_t *data);
#ifdef __cplusplus
}
#endif //__cplusplus
