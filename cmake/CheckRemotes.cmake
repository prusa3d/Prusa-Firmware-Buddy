find_package(Git QUIET)

if(NOT GIT_FOUND)
  message(STATUS "Not Git Executable found. Skipping check for dangerous Git remotes.")
  return()
endif()

function(check_git_repo_for_dangerous_remotes repo_dir)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" remote
    WORKING_DIRECTORY "${repo_dir}"
    RESULT_VARIABLE res
    OUTPUT_VARIABLE lst
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  if(NOT res STREQUAL "0")
    message(WARNING "Failed to check dangerousness of your Git remotes!")
    return()
  endif()

  string(REPLACE "\n" ";" lst ${lst})
  foreach(item ${lst})
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" remote get-url --push ${item}
      WORKING_DIRECTORY "${repo_dir}"
      RESULT_VARIABLE res
      OUTPUT_VARIABLE url
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    if(NOT res STREQUAL 0)
      message(WARNING "Failed to check dangerousness of remote '${item}'!")
    endif()

    if(url MATCHES "Prusa\-Firmware\-Buddy.git" OR url MATCHES "Marlin.git")
      message(
        FATAL_ERROR
          "Oh, your remote '${item}' appears to have its push URL set to a public repository! Let me say, that this is a bad, bad idea! You are \"one push\" away from mistakenly publishing private things. Please remove this remote or set its push url to some nonsense (see below).\n git -C \"${repo_dir}\" remote set-url --push ${item} DISABLED\n"
        )
    endif()

  endforeach()
endfunction()

check_git_repo_for_dangerous_remotes("${CMAKE_SOURCE_DIR}")
check_git_repo_for_dangerous_remotes("${CMAKE_SOURCE_DIR}/lib/Marlin")
