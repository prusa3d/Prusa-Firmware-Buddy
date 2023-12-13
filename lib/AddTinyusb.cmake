if(NOT TARGET tinyusb_dependencies)
  message(FATAL_ERROR "Target tinyusb_dependencies does not exist.")
endif()

add_library(
  tinyusb
  tinyusb/src/class/cdc/cdc_device.c
  tinyusb/src/class/cdc/cdc_host.c
  tinyusb/src/class/cdc/cdc_rndis_host.c
  tinyusb/src/class/msc/msc_device.c
  tinyusb/src/common/tusb_fifo.c
  tinyusb/src/device/usbd.c
  tinyusb/src/device/usbd_control.c
  tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
  tinyusb/src/tusb.c
  )

target_include_directories(tinyusb PUBLIC tinyusb/src)

target_compile_definitions(tinyusb PUBLIC CFG_TUSB_MCU=OPT_MCU_STM32F4)

target_link_libraries(tinyusb PUBLIC tinyusb_dependencies)

add_library(tinyusb::tinyusb ALIAS tinyusb)
