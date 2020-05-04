add_library(CRC tm_stm32f4_crc.c)
target_include_directories(CRC PUBLIC .)
target_link_libraries(CRC PUBLIC CMSIS STM32::CPU)
