if(NOT TARGET STM32_USBHost_Config)
  message(FATAL_ERROR "Target STM32_USBHost_Config does not exist.")
endif()

add_library(
  USBHost
  Class/MSC/Src/usbh_msc.c
  Class/MSC/Src/usbh_msc_bot.c
  Class/MSC/Src/usbh_msc_scsi.c
  Core/Src/usbh_core.c
  Core/Src/usbh_ctlreq.c
  Core/Src/usbh_ioreq.c
  Core/Src/usbh_pipes.c
  )

target_include_directories(USBHost PUBLIC Core/Inc Class/MSC/Inc)

target_link_libraries(USBHost PUBLIC STM32_USBHost_Config)

add_library(STM32::USBHost ALIAS USBHost)
