set(Binarize_DOWNSTREAM_DEPS "")

set(heatshrink_VER 0.4)
set(ZLIB_VER 1.0)

find_package(heatshrink ${heatshrink_VER} REQUIRED)
find_package(ZLIB ${ZLIB_VER} REQUIRED)

if (NOT BUILD_SHARED_LIBS)
    list(APPEND Binarize_DOWNSTREAM_DEPS "heatshrink_${heatshrink_VER}")
    list(APPEND Binarize_DOWNSTREAM_DEPS "ZLIB_${ZLIB_VER}")
    # append all the libs that are required privately for Core
endif ()

# Binarize component
add_library(${_libname}_binarize
    binarize.cpp
    binarize.hpp
    meatpack.cpp
    meatpack.hpp
    ${PROJECT_BINARY_DIR}/version.rc
    # Add more source files here if needed
)

add_library(${namespace}${_libname}_binarize ALIAS ${_libname}_binarize)

generate_export_header(${_libname}_binarize
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/binarize/export.h
)

target_include_directories(${_libname}_binarize
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${_srcloc}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${_libname}_binarize PRIVATE heatshrink::heatshrink_dynalloc ZLIB::ZLIB)
target_link_libraries(${_libname}_binarize PUBLIC ${_libname}_core)

set(Binarize_DOWNSTREAM_DEPS ${Binarize_DOWNSTREAM_DEPS} PARENT_SCOPE)
