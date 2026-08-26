set(Core_DOWNSTREAM_DEPS "")

# Core component
add_library(${_libname}_core
   core.cpp
   core.hpp
   core_impl.hpp
   ${PROJECT_BINARY_DIR}/version.rc
   # Add more source files here if needed
)

if (${PROJECT_NAME}_BUILD_SANITIZERS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${_libname}_core PUBLIC
            -g
            -Wstrict-aliasing
            -fsanitize=address
        )
        target_link_options(${_libname}_core PUBLIC -fsanitize=address)
    elseif (MSVC)
        target_compile_options(${_libname}_core PUBLIC /fsanitize=address /Zi)
        target_link_options(${_libname}_core PUBLIC /DEBUG)
    endif ()
endif ()

target_compile_definitions(${_libname}_core PRIVATE LibBGCode_VERSION=R"\(${LibBGCode_VERSION}\)")

generate_export_header(${_libname}_core
   EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/core/export.h
)

target_include_directories(${_libname}_core
   PUBLIC
       $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${_srcloc}>
       $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
       $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
       $<INSTALL_INTERFACE:include>
)

add_library(${namespace}${_libname}_core ALIAS ${_libname}_core)

# Set the version of the project
# target_sources(Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.h)
# target_compile_definitions(Core PRIVATE VERSION_MAJOR=${PROJECT_VERSION_MAJOR} VERSION_MINOR=${PROJECT_VERSION_MINOR})

set(Core_DOWNSTREAM_DEPS ${Core_DOWNSTREAM_DEPS} PARENT_SCOPE)
