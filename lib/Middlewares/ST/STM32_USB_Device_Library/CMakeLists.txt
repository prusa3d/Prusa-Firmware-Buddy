if(NOT TARGET STM32_USBDevice_Config)
  message(FATAL_ERROR "Target STM32_USBDevice_Config does not exist.")
endif()

add_library(
  USBDevice Class/CDC/Src/usbd_cdc.c Core/Src/usbd_core.c Core/Src/usbd_ctlreq.c
            Core/Src/usbd_ioreq.c
  )

target_include_directories(USBDevice PUBLIC Core/Inc Class/CDC/Inc)

target_link_libraries(USBDevice PUBLIC STM32_USBDevice_Config)

add_library(STM32::USBDevice ALIAS USBDevice)
