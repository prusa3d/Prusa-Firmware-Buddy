add_library(STM32H5_HAL_Config INTERFACE)
target_compile_definitions(STM32H5_HAL_Config INTERFACE STM32H503xx USE_HAL_DRIVER)
target_include_directories(STM32H5_HAL_Config INTERFACE config)

target_compile_definitions(firmware PRIVATE STM32GENERIC STM32H5 STM32H5xx)
target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/stm32h503.ld")

target_sources(firmware PRIVATE cmsis.cpp hal.cpp main.cpp stdlib.c stm32h503.s)
target_link_libraries(firmware PRIVATE printf::printf STM32H5::HAL STM32H5_HAL_Config)
