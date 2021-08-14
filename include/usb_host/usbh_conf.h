/**
  ******************************************************************************
  * @file           : usbh_conf.h
  * @version        : v1.0_Cube
  * @brief          : Header for usbh_conf.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBH_CONF__H__
    #define __USBH_CONF__H__
    #ifdef __cplusplus
extern "C" {
    #endif
/* Includes ------------------------------------------------------------------*/

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "main.h"

    #include "stm32f4xx.h"
    #include "stm32f4xx_hal.h"

    #include "log.h"

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
    #define USBH_USE_OS 1

    #define USE_HAL_HCD_REGISTER_CALLBACKS 1U

    /****************************************/
    /* #define for FS and HS identification */
    #define HOST_HS 0
    #define HOST_FS 1

    #if (USBH_USE_OS == 1)
        #include "cmsis_os.h"
        #define USBH_PROCESS_PRIO       osPriorityNormal
        #define USBH_PROCESS_STACK_SIZE ((uint16_t)320)
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

    #define USBH_UsrLog(...) log_info(USBHost, __VA_ARGS__)
    #define USBH_ErrLog(...) log_error(USBHost, __VA_ARGS__)
    #define USBH_DbgLog(...) log_debug(USBHost, __VA_ARGS__)

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
