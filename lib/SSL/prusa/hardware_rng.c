/**
  ******************************************************************************
  * @file    hardware_rng.c
  * @author  MCD Application Team
  * @version V1.2.1
  * @date    14-April-2017
  * @brief   mbedtls alternate entropy data function.
  *          the mbedtls_hardware_poll() is customized to use the STM32 RNG
  *          to generate random data, required for TLS encryption algorithms.
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Apache 2.0 license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  * https://opensource.org/licenses/Apache-2.0
  *
  ******************************************************************************
  */

#include "mbedtls_config.h"

#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT

    #include "main.h"
    #include "string.h"
    #include "stm32f4xx_hal.h"
    #include "mbedtls/entropy_poll.h"

extern RNG_HandleTypeDef hrng;

int mbedtls_hardware_poll(void *Data, unsigned char *Output, size_t Len, size_t *oLen) {
    uint32_t index;
    uint32_t randomValue;

    for (index = 0; index < Len / 4; index++) {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &randomValue) == HAL_OK) {
            *oLen += 4;
            memset(&(Output[index * 4]), (int)randomValue, 4);
        } else {
            Error_Handler();
        }
    }

    return 0;
}

#endif /*MBEDTLS_ENTROPY_HARDWARE_ALT*/
