set(GTEST_SOURCE_ROOT CACHE FILEPATH "Path to Google Test source checkout (if unset, CMake will search for a binary copy or fetch from github.)")

include(FetchContent OPTIONAL RESULT_VARIABLE FetchContentModule)

function(load_googletest)
  if ("${FetchContentModule}" STREQUAL "NOTFOUND")
    message(FATAL_ERROR "Could not find googletest installed locally, nor is
  CMake new enough to fetch from upstream. Please specify CMake
  GTEST_SOURCE_ROOT variable to path to local copy of googletest. For example,
  -DGTEST_SOURCE_ROOT=/path/to/extracted/copy/of/googletest/tarball")
  endif("${FetchContentModule}" STREQUAL "NOTFOUND")

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
    )

  # Can't find either as a module or config. Fetch it directly.
  FetchContent_GetProperties(googletest)
  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
  endif()
endfunction(load_googletest)


# Attempt to locate GTest using the CMake config files introduced in 1.8.1
find_package(GTest CONFIG)
if (GTest_FOUND)
  message(STATUS "Found GTest CMake Config")
  set(GTEST_TARGET GTest::gtest)
  set(GTEST_MAIN_TARGET GTest::gtest_main)
  set(GMOCK_TARGET GTest::gmock)
elseif(GTEST_SOURCE_ROOT)
  message(STATUS "Using googletest from ${GTEST_SOURCE_ROOT}")
  # Build GTest from the provided source directory
  add_subdirectory(${GTEST_SOURCE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/googletest)
  set(GTEST_TARGET gtest)
  set(GTEST_MAIN_TARGET gtest_main)
  set(GMOCK_TARGET gmock)
else()
  message(STATUS "Attempting to fetch googletest from github")
  load_googletest()
  set(GTEST_TARGET gtest)
  set(GTEST_MAIN_TARGET gtest_main)
  set(GMOCK_TARGET gmock)
endif()
