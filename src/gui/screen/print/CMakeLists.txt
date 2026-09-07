target_sources(firmware PRIVATE frame_compat_checks.cpp)

if(HAS_MMU2 OR HAS_TOOLCHANGER)
  target_sources(firmware PRIVATE frame_tool_mapping.cpp)
endif()
