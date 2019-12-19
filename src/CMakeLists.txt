cmake_minimum_required(VERSION 3.13)

project(A3ides)
enable_language(ASM)

include("CProjectDefines.cmake")
if(NOT BUILD_VARIANT)
  set(BUILD_VARIANT "DEBUG")
endif()

if(${BUILD_VARIANT} STREQUAL "DEBUG")
  add_compile_options("-Og" "-ggdb")
endif()

add_compile_options(
  "-mthumb" "-mcpu=cortex-m4" "-mfloat-abi=hard" "-mfpu=fpv4-sp-d16" "-ffunction-sections"
  "-fdata-sections"
  )

add_executable(firmware)
target_include_directories(firmware PRIVATE Common Inc ../GuiAPI/include)
target_sources(firmware PRIVATE ${CPROJECT_${BUILD_VARIANT}_SOURCES})
target_compile_definitions(firmware PRIVATE ${CPROJECT_${BUILD_VARIANT}_C_COMPILER_MACROS})
target_include_directories(firmware PRIVATE "${CPROJECT_${BUILD_VARIANT}_C_COMPILER_INCLUDES}")
target_include_directories(firmware PRIVATE "${CPROJECT_${BUILD_VARIANT}_CPP_COMPILER_INCLUDES}")

set_property(TARGET firmware PROPERTY CXX_STANDARD 11)
set_target_properties(
  firmware
  PROPERTIES
    LINK_FLAGS
    "-mcpu=cortex-m4 -mthumb -Wall -Wextra -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections -Wl,--undefined=uxTopUsedPriority,--undefined=init_task,-T,${CPROJECT_${BUILD_VARIANT}_LINKER_SCRIPT}"
  )
