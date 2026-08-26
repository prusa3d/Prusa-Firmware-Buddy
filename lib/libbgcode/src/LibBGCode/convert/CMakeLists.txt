set(Convert_DOWNSTREAM_DEPS "")

set(Boost_VER 1.78)
find_package(Boost ${Boost_VER} REQUIRED)
if (NOT BUILD_SHARED_LIBS)
    list(APPEND Convert_DOWNSTREAM_DEPS "Boost_${Boost_VER}")
    # append all the libs that are required privately for Core
endif ()

# Convert component
add_library(${_libname}_convert
    convert.cpp
    convert.hpp
    ${PROJECT_BINARY_DIR}/version.rc
    # Add more source files here if needed
)

add_library(${namespace}${_libname}_convert ALIAS ${_libname}_convert)

generate_export_header(${_libname}_convert
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/convert/export.h
)

target_include_directories(${_libname}_convert
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${_srcloc}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${_libname}_convert PUBLIC ${_libname}_binarize ${_libname}_core)
target_link_libraries(${_libname}_convert PRIVATE Boost::boost)

set(Convert_DOWNSTREAM_DEPS ${Convert_DOWNSTREAM_DEPS} PARENT_SCOPE)
