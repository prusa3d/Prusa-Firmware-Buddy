# NFCV helper library that implements the generic encoding/decoding of NFC-V commands

add_library(nfcv OBJECT commands.cpp encode.cpp decode.cpp rw_interface.cpp)

target_include_directories(nfcv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nfcv PUBLIC iso13239 SG14 buddy_utils bsod)
