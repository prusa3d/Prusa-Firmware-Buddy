add_library(CMSIS_common INTERFACE)
target_include_directories(CMSIS_common INTERFACE Include)

add_library(STM32F4xx_CMSIS INTERFACE)
target_include_directories(STM32F4xx_CMSIS INTERFACE Device/ST/STM32F4xx/Include)
target_link_libraries(STM32F4xx_CMSIS INTERFACE CMSIS_common)
add_library(STM32F4xx::CMSIS ALIAS STM32F4xx_CMSIS)

add_library(STM32G0xx_CMSIS INTERFACE)
target_include_directories(STM32G0xx_CMSIS INTERFACE Include Device/ST/STM32G0xx/Include)
target_link_libraries(STM32G0xx_CMSIS INTERFACE CMSIS_common)
add_library(STM32G0xx::CMSIS ALIAS STM32G0xx_CMSIS)
