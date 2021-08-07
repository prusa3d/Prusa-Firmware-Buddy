if(NOT TARGET Segger_Config)
  message(FATAL_ERROR "Target Segger_Config does not exist.")
endif()

add_library(
  Segger Segger/SEGGER_RTT.c Segger/SEGGER_RTT_ASM_ARMv7M.S Segger/SEGGER_SYSVIEW.c
         Segger/SEGGER_SYSVIEW_Int.h
  )

target_include_directories(Segger PUBLIC Segger)

target_link_libraries(Segger PUBLIC Segger_Config)
