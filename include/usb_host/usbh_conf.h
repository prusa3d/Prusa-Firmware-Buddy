/**
  ******************************************************************************
  * @file           : usbh_conf.h
  * @version        : v1.0_Cube
  * @brief          : Header for usbh_conf.c file.
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V.
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBH_CONF__H__
    #define __USBH_CONF__H__
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #ifdef __cplusplus
extern "C" {
    #endif

    /* Includes ------------------------------------------------------------------*/
    #include "stm32f4xx.h"
    #include "stm32f4xx_hal.h"

    /* USER CODE BEGIN INCLUDE */

    /* USER CODE END INCLUDE */

    /** @addtogroup USBH_OTG_DRIVER
  * @brief Driver for Usb host.
  * @{
  */

    /** @defgroup USBH_CONF USBH_CONF
  * @brief Configuration file for Usb otg low level driver.
  * @{
  */

    /** @defgroup USBH_CONF_Exported_Variables USBH_CONF_Exported_Variables
  * @brief Public variables.
  * @{
  */

    /**
  * @}
  */

    /** @defgroup USBH_CONF_Exported_Defines USBH_CONF_Exported_Defines
  * @brief Defines for configuration of the Usb host.
  * @{
  */

    /*
	MiddleWare name : USB_HOST
	MiddleWare fileName : usbh_conf.h
	MiddleWare version :
*/
    /*----------   -----------*/
    #define USBH_MAX_NUM_ENDPOINTS 2

    /*----------   -----------*/
    #define USBH_MAX_NUM_INTERFACES 2

    /*----------   -----------*/
    #define USBH_MAX_NUM_CONFIGURATION 1

    /*----------   -----------*/
    #define USBH_KEEP_CFG_DESCRIPTOR 1

    /*----------   -----------*/
    #define USBH_MAX_NUM_SUPPORTED_CLASS 1

    /*----------   -----------*/
    #define USBH_MAX_SIZE_CONFIGURATION 256

    /*----------   -----------*/
    #define USBH_MAX_DATA_BUFFER 512

    /*----------   -----------*/
    #define USBH_DEBUG_LEVEL 0

    /*----------   -----------*/
    #define USBH_USE_OS 1

    /****************************************/
    /* #define for FS and HS identification */
    #define HOST_HS 0
    #define HOST_FS 1

    #if (USBH_USE_OS == 1)
        #include "cmsis_os.h"
        #define USBH_PROCESS_PRIO       osPriorityNormal
        #define USBH_PROCESS_STACK_SIZE ((uint16_t)128)
    #endif /* (USBH_USE_OS == 1) */

    /**
  * @}
  */

    /** @defgroup USBH_CONF_Exported_Macros USBH_CONF_Exported_Macros
  * @brief Aliases.
  * @{
  */

    /* Memory management macros */

    /** Alias for memory allocation. */
    #define USBH_malloc malloc

    /** Alias for memory release. */
    #define USBH_free free

    /** Alias for memory set. */
    #define USBH_memset memset

    /** Alias for memory copy. */
    #define USBH_memcpy memcpy

/* DEBUG macros */

    #if (USBH_DEBUG_LEVEL > 0)
        #define USBH_UsrLog(...) \
            printf(__VA_ARGS__); \
            printf("\n");
    #else
        #define USBH_UsrLog(...)
    #endif

    #if (USBH_DEBUG_LEVEL > 1)

        #define USBH_ErrLog(...) \
            printf("ERROR: ");   \
            printf(__VA_ARGS__); \
            printf("\n");
    #else
        #define USBH_ErrLog(...)
    #endif

    #if (USBH_DEBUG_LEVEL > 2)
        #define USBH_DbgLog(...) \
            printf("DEBUG : ");  \
            printf(__VA_ARGS__); \
            printf("\n");
    #else
        #define USBH_DbgLog(...)
    #endif

/**
  * @}
  */

/** @defgroup USBH_CONF_Exported_Types USBH_CONF_Exported_Types
  * @brief Types.
  * @{
  */

/**
  * @}
  */

/** @defgroup USBH_CONF_Exported_FunctionsPrototype USBH_CONF_Exported_FunctionsPrototype
  * @brief Declaration of public functions for Usb host.
  * @{
  */

/* Exported functions -------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

    #ifdef __cplusplus
}
    #endif

#endif /* __USBH_CONF__H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
