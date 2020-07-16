
# define the test executable
add_executable(translator_tests
../../../../src/lang/translator.cpp
../../../../src/lang/translation_provider_CPUFLASH.cpp
../../../../src/lang/unaccent.cpp
../../test_main.cpp
providerCPUFLASH.cpp
hash.cpp
)

# define required search paths
target_include_directories(translator_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/lang)

# define custom macros
target_compile_definitions(translator_tests PUBLIC TRANSLATIONS_UNITTEST)

# tell build system about the test case
add_catch_test(translator_tests)
