if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(NOT LLMV_PROFDATA)
    set(LLMV_PROFDATA llvm-profdata)
  endif()
  if(NOT LLVM_COV)
    set(LLMV_COV llvm-cov)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(NOT LLMV_PROFDATA)
    set(LLMV_PROFDATA xcrun llvm-profdata)
  endif()
  if(NOT LLVM_COV)
    set(LLMV_COV xcrun llvm-cov)
  endif()
endif()

function(enable_coverage)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(LLMV_PROFDATA xcrun llvm-profdata)
    set(LLMV_COV xcrun llvm-cov)
  else()
    message(
      FATAL_ERROR
        "Source level code coverage is supported only for Clang compiler! (CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID})"
      )
  endif()

  message(STATUS "Enabling Clang source code-coverage")

  # add flags to emit coverage
  add_compile_options("-fprofile-instr-generate" "-fcoverage-mapping" "-g" "-O0")
  add_link_options("-fprofile-instr-generate" "-fcoverage-mapping")
  add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=/")
endfunction()

function(coverage_report_after EVENT TARGET)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # ok
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

  else()
    return()
  endif()

  set(coverage_data_name "default.profraw")

  add_custom_command(
    TARGET ${EVENT}
    POST_BUILD
    COMMAND ${LLMV_PROFDATA} merge -sparse ${coverage_data_name} -o coverage.profdata
    COMMAND
      ${LLMV_COV} show $<TARGET_FILE:${TARGET}> -instr-profile=coverage.profdata
      "-ignore-filename-regex=\"(external/.*|tests/.*|cthash/internal/assert[.]hpp)\"" -format html
      -output-dir ${CMAKE_BINARY_DIR}/report -show-instantiations=true -show-expansions=false
      -show-line-counts -Xdemangler c++filt -Xdemangler -n -show-branches=percent -tab-size=4
      -path-equivalence=/,${CMAKE_SOURCE_DIR}
    COMMAND cd ${CMAKE_BINARY_DIR} && zip -q -r -9 report.zip report
    BYPRODUCTS ${CMAKE_BINARY_DIR}/report.zip
    COMMENT "Generating Code-Coverage report"
    )

  add_custom_target(
    coverage
    COMMAND open ${CMAKE_BINARY_DIR}/report/index.html
    DEPENDS ${CMAKE_BINARY_DIR}/report.zip
    )
endfunction()
