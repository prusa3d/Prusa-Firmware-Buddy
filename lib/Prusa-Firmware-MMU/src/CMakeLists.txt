target_sources(firmware PRIVATE main.cpp debug.cpp)

target_link_libraries(firmware LUFA)

set_property(
  SOURCE src/version.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS
           FW_BUILD_NUMBER=${BUILD_NUMBER}
           FW_VERSION_FULL=${PROJECT_VERSION_FULL}
           FW_VERSION=${PROJECT_VERSION}
           FW_VERSION_SUFFIX=${PROJECT_VERSION_SUFFIX}
           FW_VERSION_SUFFIX_SHORT=${PROJECT_VERSION_SUFFIX_SHORT}
  )

add_subdirectory(hal)
add_subdirectory(logic)
add_subdirectory(modules)
