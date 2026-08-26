cmake_minimum_required(VERSION 3.12)
project(LibBGCode_Deps)

# Support legacy parameter DESTDIR
if (DESTDIR)
  set(${PROJECT_NAME}_DEP_INSTALL_PREFIX ${DESTDIR}/usr/local CACHE PATH "Destination directory" FORCE)
endif ()

# Support legacy parameter DEP_DOWNLOAD_DIR
if (DEP_DOWNLOAD_DIR)
  set(${PROJECT_NAME}_DEP_DOWNLOAD_DIR ${DEP_DOWNLOAD_DIR} CACHE PATH "Path for downloaded source packages." FORCE)
endif ()

# Slightly controversial
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
endif ()

if (APPLE)
    # This ensures dependencies don't use SDK features which are not available in the version specified by Deployment target
    # That can happen when one uses a recent SDK but specifies an older Deployment target
    set(DEP_WERRORS_SDK "-Werror=partial-availability -Werror=unguarded-availability -Werror=unguarded-availability-new")

    set(DEP_CMAKE_OPTS
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
        "-DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}"
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
        "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}"
        "-DCMAKE_CXX_FLAGS=${DEP_WERRORS_SDK}"
        "-DCMAKE_C_FLAGS=${DEP_WERRORS_SDK}"
        "-DCMAKE_FIND_FRAMEWORK=LAST"
        "-DCMAKE_FIND_APPBUNDLE=LAST"
    )
endif ()

if (EMSCRIPTEN)
    set(DEP_CMAKE_OPTS "-DCMAKE_CXX_FLAGS=-fms-extensions")
endif ()

include(${PROJECT_SOURCE_DIR}/../cmake/modules/AddCMakeProject.cmake)

set(supported_packages Boost heatshrink Catch2 ZLIB pybind11)

include(CMakeDependentOption)
option(${PROJECT_NAME}_SELECT_ALL "Choose all external projects to be selected (create build target for them)." ON)
option(${PROJECT_NAME}_BUILD_ALL "Choose all external projects to be built." OFF)

find_package(Git REQUIRED)

# The default command line for patching. Only works for newer
set(PATCH_CMD ${GIT_EXECUTABLE} apply --verbose --ignore-space-change --whitespace=fix)

foreach (pkg ${supported_packages})
    option(${PROJECT_NAME}_BUILD_${pkg} "Select package ${pkg} to be built." OFF)
    if (${PROJECT_NAME}_SELECT_ALL OR ${PROJECT_NAME}_BUILD_ALL OR ${PROJECT_NAME}_BUILD_${pkg})
        include(${pkg}/${pkg}.cmake)
    endif ()
endforeach()

install(CODE "message(STATUS \"Built packages succesfully.\")")
