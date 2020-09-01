
# define the test executable
add_executable(translator_tests
    ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp
    ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_CPUFLASH.cpp
    ${CMAKE_SOURCE_DIR}/src/lang/unaccent.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providerCPUFLASH.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hash.cpp
)

# define required search paths
target_include_directories(translator_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/lang
)

# define custom macros
target_compile_definitions(translator_tests PUBLIC TRANSLATIONS_UNITTEST)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cs.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/de.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/es.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fr.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pl.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/it.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/keys.txt ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# tell build system about the test case
add_catch_test(translator_tests)
