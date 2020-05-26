set(BENCHMARK_SOURCE_ROOT CACHE FILEPATH "Path to Google Benchmark source checkout (if unset, CMake will search for a binary copy or fetch from github.)")

include(FetchContent OPTIONAL RESULT_VARIABLE FetchContentModule)

function(load_benchmark)
  if ("${FetchContentModule}" STREQUAL "NOTFOUND")
    message(FATAL_ERROR "Could not find benchmark installed locally, nor is
  CMake new enough to fetch from upstream. Please specify CMake
  BENCHMARK_SOURCE_ROOT variable to path to local copy of benchmark. For example,
  -DBENCHMARK_SOURCE_ROOT=/path/to/extracted/copy/of/benchmark/tarball")
  endif("${FetchContentModule}" STREQUAL "NOTFOUND")

  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.5.0
    )

  # Can't find either as a module or config. Fetch it directly.
  FetchContent_GetProperties(benchmark)
  if(NOT benchmark_POPULATED)
    FetchContent_Populate(benchmark)
    add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})
  endif()
endfunction(load_benchmark)


find_package(benchmark CONFIG)
if (benchmark_FOUND)
  set(BENCHMARK_TARGET benchmark::benchmark)
else()
  # The benchmark library is not installed on the system. We have to build it.
  # Disable testing of the benchmark library itself (requires googletest sources).
  option(BENCHMARK_ENABLE_TESTING "Enable testing of the benchmark library" OFF)
  if(BENCHMARK_SOURCE_ROOT)
    # Build benchmark from the provided source directory
    add_subdirectory(${BENCHMARK_SOURCE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/benchmark)
  else()
    load_benchmark()
  endif()
  set(BENCHMARK_TARGET benchmark)
endif()
