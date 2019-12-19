if(NOT TARGET STM32_Utilities_CPU_Config)
  message(FATAL_ERROR "Target STM32_Utilities_CPU_Config does not exist.")
endif()

add_library(CPU cpu_utils.c)

target_include_directories(CPU PUBLIC .)

target_link_libraries(CPU PUBLIC STM32_Utilities_CPU_Config)

add_library(STM32::CPU ALIAS CPU)
