# getlocked version
get_filename_component(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(PROJECT_ROOT_DIR "${PROJECT_CMAKE_DIR}" DIRECTORY)
include("${PROJECT_CMAKE_DIR}/Utilities.cmake")

get_recommended_gcc_version(RECOMMENDED_TOOLCHAIN_VERSION)
set(RECOMMENDED_TOOLCHAIN_BINUTILS
    "${PROJECT_ROOT_DIR}/.dependencies/gcc-arm-none-eabi-${RECOMMENDED_TOOLCHAIN_VERSION}/bin"
    )

# check that the locked version of gcc-arm-none-eabi is present
if(NOT EXISTS "${RECOMMENDED_TOOLCHAIN_BINUTILS}")
  message(
    FATAL_ERROR
      "arm-none-eabi-gcc (version ${RECOMMENDED_TOOLCHAIN_VERSION}) not found. Run the command below to download it.\n"
      "${PROJECT_ROOT_DIR}/utils/bootstrap.sh\n"
    )
endif()

# include any-gcc-arm-none-eabi toolchain and pass in ARM_TOOLCHAIN_DIR
get_filename_component(ARM_TOOLCHAIN_DIR "${RECOMMENDED_TOOLCHAIN_BINUTILS}" DIRECTORY)
include("${PROJECT_ROOT_DIR}/cmake/AnyGccArmNoneEabi.cmake")
