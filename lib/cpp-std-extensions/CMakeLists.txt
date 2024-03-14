cmake_minimum_required(VERSION 3.21)

project(
    stdx
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A header-only C++ library that polyfills the standard library."
)

include(cmake/get_cpm.cmake)
cpmaddpackage("gh:intel/cicd-repo-infrastructure#main")

add_versioned_package("gh:boostorg/mp11#boost-1.83.0")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

add_library(stdx INTERFACE)
target_include_directories(stdx INTERFACE include)
target_compile_features(stdx INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_compile_options(
    stdx
    INTERFACE
        $<$<CXX_COMPILER_ID:Clang>:-Wno-gnu-string-literal-operator-template>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-braces>)
target_link_libraries_system(stdx INTERFACE boost_mp11)

if(PROJECT_IS_TOP_LEVEL)
    add_docs(docs)
    add_subdirectory(test)
    clang_tidy_interface(stdx)
endif()
