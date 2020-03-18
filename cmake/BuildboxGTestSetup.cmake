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
  set(GTEST_TARGET GTest::gtest)
  set(GMOCK_TARGET GTest::gmock)
else()
  # Attempt to locate GTest using CMake's FindGTest module
  find_package(GTest MODULE)
  if (NOT GTest_FOUND)
    if(GTEST_SOURCE_ROOT)
      # Build GTest from the provided source directory
      add_subdirectory(${GTEST_SOURCE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/googletest)
      set(GTEST_TARGET gtest)
      set(GMOCK_TARGET gmock)
    else()
      load_googletest()
      set(GTEST_TARGET gtest)
      set(GMOCK_TARGET gmock)
    endif()
  else()
    # CMake's FindGTest module doesn't include easy support for locating GMock
    # (see https://gitlab.kitware.com/cmake/cmake/issues/17365),
    # so we hack it in by replacing "gtest" with "gmock" in the GTest configuration
    # it found. This works because GMock is included in recent versions of GTest.

    string(REPLACE "gtest" "gmock" GMOCK_LIBRARY ${GTEST_LIBRARY})
    string(REPLACE "gtest" "gmock" GMOCK_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS})

    add_library(GMock::GMock UNKNOWN IMPORTED)
    set_target_properties(GMock::GMock PROPERTIES
      IMPORTED_LOCATION ${GMOCK_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES "${GMOCK_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES GTest::GTest
      )

    set(GTEST_TARGET GTest::GTest)
    set(GMOCK_TARGET GMock::GMock)
  endif()
endif()
