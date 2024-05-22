#
# This file is responsible for setting the following variables:
#
# ~~~
# BUILD_NUMBER (1035)
# PROJECT_VERSION (4.0.3)
# PROJECT_VERSION_FULL (4.0.3-BETA+1035.PR111.B4)
# PROJECT_VERSION_SUFFIX (-BETA+1035.PR111.B4)
# PROJECT_VERSION_SUFFIX_SHORT (+1035)
# PROJECT_VERSION_TIMESTAMP (unix timestamp)
#
# The `PROJECT_VERSION` variable is set as soon as the file is included.
# To set the rest, the function `resolve_version_variables` has to be called.
#
# ~~~

include(${CMAKE_SOURCE_DIR}/version.cmake)

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_REV}")

function(resolve_version_variables)
  # BUILD_NUMBER
  if(NOT BUILD_NUMBER)
    git_count_parent_commits(BUILD_NUMBER)
    set(ERRORS "GIT-NOTFOUND" "HEAD-HASH-NOTFOUND")
    if(BUILD_NUMBER IN_LIST ERRORS)
      message(WARNING "Failed to resolve build number: ${BUILD_NUMBER}. Setting to zero.")
      set(BUILD_NUMBER "0")
    endif()
    set(BUILD_NUMBER
        ${BUILD_NUMBER}
        PARENT_SCOPE
        )
  endif()

  # PROJECT_VERSION_SUFFIX
  if(PROJECT_VERSION_SUFFIX STREQUAL "<auto>")
    # TODO: set to +<sha>.dirty?.debug?
    set(PROJECT_VERSION_SUFFIX "+${BUILD_NUMBER}.LOCAL")
    set(PROJECT_VERSION_SUFFIX
        "+${BUILD_NUMBER}.LOCAL"
        PARENT_SCOPE
        )
  endif()

  # PROJECT_VERSION_SUFFIX_SHORT
  if(PROJECT_VERSION_SUFFIX_SHORT STREQUAL "<auto>")
    set(PROJECT_VERSION_SUFFIX_SHORT
        "+${BUILD_NUMBER}"
        PARENT_SCOPE
        )
  endif()

  # PROJECT_VERSION_FULL
  set(PROJECT_VERSION_FULL
      "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}"
      PARENT_SCOPE
      )

  # PROJECT_VERSION_TIMESTAMP
  if(NOT PROJECT_VERSION_TIMESTAMP)
    git_head_commit_timestamp(timestamp)
    set(ERRORS "GIT-NOTFOUND" "HEAD-FORMAT-NOTFOUND" "HEAD-HASH-NOTFOUND")
     if(timestamp IN_LIST ERRORS)
       # git not available, set fallback values
       set(timestamp 0)
     endif()
    set(PROJECT_VERSION_TIMESTAMP
        "${timestamp}"
        PARENT_SCOPE
        )
  endif()

endfunction()
