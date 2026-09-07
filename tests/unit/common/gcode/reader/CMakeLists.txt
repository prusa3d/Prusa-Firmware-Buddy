add_library(
  gcode_reader_test_base OBJECT
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_buffer.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_any.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_binary.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_interface.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_reader_plaintext.cpp
  ${CMAKE_SOURCE_DIR}/src/common/gcode/meatpack.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/path_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/stat_retry.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/e2ee.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/key.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/sha256_multiuse.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/movable_aes_context.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/decryptor.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/e2ee/hmac.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crash_dump/secret.cpp
  ${CMAKE_SOURCE_DIR}/lib/heatshrink/heatshrink_decoder.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/async_job/async_job.cpp
  missing_functions.c
  )
target_include_directories(
  gcode_reader_test_base
  PUBLIC .
         ${CMAKE_CURRENT_BINARY_DIR}
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/common/utils
         ${CMAKE_SOURCE_DIR}/src
         ${CMAKE_SOURCE_DIR}/src/common/gcode
         ${CMAKE_SOURCE_DIR}/lib
         ${CMAKE_SOURCE_DIR}/lib/heatshrink
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_SOURCE_DIR}/include/usb_host
  )

target_link_libraries(
  gcode_reader_test_base
  PUBLIC SG14
         mbedTLS
         bgcode_core
         freertos_tests
         raii
         logging-mock
         buddy_utils
         freertos-mock
         random
         random_sw
         base64
  )

add_executable(gcode_reader_test gcode_reader.cpp)
target_link_libraries(gcode_reader_test gcode_reader_test_base)
add_catch_test(gcode_reader_test)

add_executable(
  gcode_info_test
  ${CMAKE_SOURCE_DIR}/src/common/gcode/gcode_info.cpp
  ${CMAKE_SOURCE_DIR}/src/common/printer_model.cpp ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/color.cpp gcode_info.cpp
  )
target_include_directories(
  gcode_info_test
  PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs # Must be before lib/Marlin include
         ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src ${CMAKE_SOURCE_DIR}/src/lang
         ${CMAKE_SOURCE_DIR}/src/common
  )
target_link_libraries(gcode_info_test gcode_reader_test_base version)
add_catch_test(gcode_info_test)

file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_plain.gcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_total_toolchanges.gcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_no_compression.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_meatpack.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_heatshrink.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_binary_heatshrink_meatpack.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_first_gcode.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_intro.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_bad_crc_gcode.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_correct.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_decrypted_gcode_correct.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/printer_private_key.der
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_diff_printer.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_bad_key_hash.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_metadata_not_beggining.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_key_before_identity.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_encrypted_before_identity.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_encrypted_before_key.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_plain_gcode_inside_encrypted.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_invalid_identity_key.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_bad_identity_block_signature.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_corrupted_metadata.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_early_last_block.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/test_encrypted_gcode_no_last_block.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new.gcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_meta_after.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_meta_before.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_enc.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_enc_multiple.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_enc_poly.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_enc_fully.bgcode
       ${CMAKE_CURRENT_SOURCE_DIR}/box_new_signed.bgcode
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )
