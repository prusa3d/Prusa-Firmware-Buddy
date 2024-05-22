get_filename_component(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
include("${PROJECT_CMAKE_DIR}/Utilities.cmake")
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

#
# Utilities

if(MINGW
   OR CYGWIN
   OR WIN32
   )
  set(UTIL_SEARCH_CMD where)
  set(EXECUTABLE_SUFFIX ".exe")
elseif(UNIX OR APPLE)
  set(UTIL_SEARCH_CMD which)
  set(EXECUTABLE_SUFFIX "")
endif()

set(TOOLCHAIN_PREFIX arm-none-eabi-)

#
# Looking up the toolchain
#

if(ARM_TOOLCHAIN_DIR)
  # using toolchain set by gcc-arm-none-eabi.cmake (locked version)
  set(BINUTILS_PATH "${ARM_TOOLCHAIN_DIR}/bin")
else()
  # search for ANY arm-none-eabi-gcc toolchain
  execute_process(
    COMMAND ${UTIL_SEARCH_CMD} ${TOOLCHAIN_PREFIX}gcc
    OUTPUT_VARIABLE ARM_NONE_EABI_GCC_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE FIND_RESULT
    )
  # found?
  if(NOT "${FIND_RESULT}" STREQUAL "0")
    message(FATAL_ERROR "arm-none-eabi-gcc not found")
  endif()
  get_filename_component(BINUTILS_PATH "${ARM_NONE_EABI_GCC_PATH}" DIRECTORY)
  get_filename_component(ARM_TOOLCHAIN_DIR ${BINUTILS_PATH} DIRECTORY)
endif()

#
# Setup CMake
#

# Without that flag CMake is not able to pass test compilation check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(triple armv7m-none-eabi)
set(CMAKE_C_COMPILER
    "${BINUTILS_PATH}/${TOOLCHAIN_PREFIX}gcc${EXECUTABLE_SUFFIX}"
    CACHE FILEPATH "" FORCE
    )
set(CMAKE_C_COMPILER_TARGET
    ${triple}
    CACHE STRING "" FORCE
    )
set(CMAKE_ASM_COMPILER
    "${BINUTILS_PATH}/${TOOLCHAIN_PREFIX}gcc${EXECUTABLE_SUFFIX}"
    CACHE FILEPATH "" FORCE
    )
set(CMAKE_ASM_COMPILER_TARGET
    ${triple}
    CACHE STRING "" FORCE
    )
set(CMAKE_CXX_COMPILER
    "${BINUTILS_PATH}/${TOOLCHAIN_PREFIX}g++${EXECUTABLE_SUFFIX}"
    CACHE FILEPATH "" FORCE
    )
set(CMAKE_CXX_COMPILER_TARGET
    ${triple}
    CACHE STRING "" FORCE
    )

set(SPECS "--specs=nosys.specs --specs=nano.specs")
set(CMAKE_C_FLAGS_INIT
    "${SPECS}"
    CACHE STRING "" FORCE
    )
set(CMAKE_CXX_FLAGS_INIT
    "${SPECS}"
    CACHE STRING "" FORCE
    )

set(CMAKE_ASM_COMPILE_OBJECT
    "<CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>"
    CACHE STRING "" FORCE
    )

set(CMAKE_OBJCOPY
    "${BINUTILS_PATH}/${TOOLCHAIN_PREFIX}objcopy${EXECUTABLE_SUFFIX}"
    CACHE INTERNAL "objcopy tool"
    )
set(CMAKE_SIZE_UTIL
    "${BINUTILS_PATH}/${TOOLCHAIN_PREFIX}size${EXECUTABLE_SUFFIX}"
    CACHE INTERNAL "size tool"
    )

set(CMAKE_FIND_ROOT_PATH "${ARM_TOOLCHAIN_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
