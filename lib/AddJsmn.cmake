add_library(jsmn INTERFACE)

target_include_directories(jsmn INTERFACE jsmn)

add_library(jsmn::jsmn ALIAS jsmn)
