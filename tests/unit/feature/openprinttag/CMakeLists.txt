set(OPT_SRC "${CMAKE_SOURCE_DIR}/src/feature/openprinttag")

# buddy_openprinttag_multirequest_tests
add_executable(
  buddy_openprinttag_multirequest_tests
  ${OPT_SRC}/detail/requests_multi_base.cpp ${OPT_SRC}/tool_tag.cpp
  buddy_openprinttag_multirequest_tests.cpp buddy_openprinttag_request_stubs.cpp
  )
target_include_directories(
  buddy_openprinttag_multirequest_tests PRIVATE stub_include/tool_tag ${CMAKE_SOURCE_DIR}/src
                                                ${CMAKE_SOURCE_DIR}/src/common
  )
target_link_libraries(
  buddy_openprinttag_multirequest_tests
  openprinttag_fields
  buddy_utils
  anfc
  freertos_tests
  logging-mock
  uuid
  )
add_catch_test(buddy_openprinttag_multirequest_tests)

# buddy_openprinttag_datautils_tests
add_executable(
  buddy_openprinttag_datautils_tests
  ${CMAKE_SOURCE_DIR}/src/common/filament_common.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filament_list.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filament_presets.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/string_builder.cpp
  ${OPT_SRC}/data_utils.cpp
  ${OPT_SRC}/detail/requests_multi_base.cpp
  ${OPT_SRC}/tool_tag.cpp
  buddy_openprinttag_datautils_tests.cpp
  buddy_openprinttag_request_stubs.cpp
  )
target_include_directories(
  buddy_openprinttag_datautils_tests
  PRIVATE stub_include/data_utils stub_include/tool_tag stub_include/requests
          ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
  )
target_link_libraries(
  buddy_openprinttag_datautils_tests
  openprinttag_fields
  buddy_utils
  anfc
  freertos_tests
  logging-mock
  uuid
  mbedTLS
  )
add_catch_test(buddy_openprinttag_datautils_tests)

# filament_usage_tracker_tests
add_executable(
  filament_usage_tracker_tests
  ${CMAKE_SOURCE_DIR}/tests/stubs/async_job/async_job.cpp
  ${OPT_SRC}/filament_usage_tracker/filament_usage_tracker.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/string_builder.cpp
  ${OPT_SRC}/detail/requests_multi_base.cpp
  ${OPT_SRC}/tool_tag.cpp
  ${OPT_SRC}/data_utils.cpp
  filament_usage_tracker_tests.cpp
  buddy_openprinttag_request_stubs.cpp
  )
target_include_directories(
  filament_usage_tracker_tests
  PRIVATE stub_include/requests
          stub_include/tool_tag
          stub_include/filament_usage_tracker
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common
  )
target_link_libraries(
  filament_usage_tracker_tests
  openprinttag_fields
  buddy_utils
  raii
  logging-mock
  freertos-mock
  anfc
  uuid
  mbedTLS
  )
add_catch_test(filament_usage_tracker_tests)
