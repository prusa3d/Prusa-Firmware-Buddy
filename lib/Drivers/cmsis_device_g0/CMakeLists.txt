add_library(CMSIS_G0 INTERFACE)

target_include_directories(CMSIS_G0 INTERFACE Include ../CMSIS/Include)

add_library(STM32G0::CMSIS ALIAS CMSIS_G0)
