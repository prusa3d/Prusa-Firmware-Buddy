#
# This file is responsible for setting the following variables:
#
# ~~~
# BUILD_NUMBER (1035)
# PROJECT_VERSION (4.0.3)
# PROJECT_VERSION_FULL (4.0.3-BETA+1035.PR111.B4)
# PROJECT_VERSION_SUFFIX (-BETA+1035.PR111.B4)
# PROJECT_VERSION_SUFFIX_SHORT (-BETA+1035)
#
# The `PROJECT_VERSION` variable is set as soon as the file is included.
# To set the rest, the function `resolve_version_variables` has to be called.
#
# ~~~

# PROJECT_VERSION
file(READ "${CMAKE_SOURCE_DIR}/version.txt" content)
string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" result "${content}")
if(NOT result)
  message(FATAL_ERROR "Failed to read version info from ${version_file}")
endif()
set(PROJECT_VERSION ${CMAKE_MATCH_0})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})

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

  # FW_COMMIT_DIRTY
  git_local_changes(IS_DIRTY)
  if(${IS_DIRTY} STREQUAL "DIRTY")
    set(FW_COMMIT_DIRTY
        TRUE
        PARENT_SCOPE
        )
  else()
    set(FW_COMMIT_DIRTY
        FALSE
        PARENT_SCOPE
        )
  endif()

  # FW_COMMIT_HASH
  get_git_head_revision(COMMIT_REFSPEC COMMIT_HASH)
  set(FW_COMMIT_HASH
      ${COMMIT_HASH}
      PARENT_SCOPE
      )
endfunction()
