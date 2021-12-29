add_executable(
  wui_utils_tests ${CMAKE_CURRENT_SOURCE_DIR}/json_encode_tests.cpp
                  ${CMAKE_SOURCE_DIR}/lib/WUI/json_encode.c
  )
target_include_directories(wui_utils_tests PRIVATE . ${CMAKE_SOURCE_DIR}/lib/WUI)

add_catch_test(wui_utils_tests)

add_subdirectory(automata)
# FIXME: Disabled for now. We ripped out the old http server, which broke some of the tests. Also
# the ones that are here need a bit of refactoring after a multipart-upload cleanup.
#
# We are keeping this around for now as part of the mocks may/will be useful. add_subdirectory(http)
