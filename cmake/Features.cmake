set(FEATURES_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")

function(define_feature name is_enabled)
  set(${name}
      ${is_enabled}
      PARENT_SCOPE
      )
  set(FEATURE_NAME ${name})
  if(is_enabled)
    set(FEATURE_ENABLED "1")
    message(STATUS "Feature ${name}: Enabled")
  else()
    set(FEATURE_ENABLED "0")
    message(STATUS "Feature ${name}: Disabled")
  endif()
  string(TOLOWER ${name} name_low)
  configure_file(
    "${CMAKE_SOURCE_DIR}/include/feature/feature.h.in"
    "${FEATURES_INCLUDE_DIR}/feature/${name_low}.h"
    )
endfunction()
