add_library(CMSIS INTERFACE)

target_include_directories(CMSIS INTERFACE Include Device/ST/STM32F4xx/Include)

add_library(STM32F4::CMSIS ALIAS CMSIS)