set(GLOG_SOURCE_ROOT CACHE FILEPATH "Path to glog source checkout (if unset, CMake will search for a binary copy or fetch from github.)")

include(FetchContent OPTIONAL RESULT_VARIABLE FetchContentModule)

function(load_glog)
  if ("${FetchContentModule}" STREQUAL "NOTFOUND")
    message(FATAL_ERROR "Could not find glog installed locally, nor is
  CMake new enough to fetch from upstream. Please specify CMake
  GLOG_SOURCE_ROOT variable to path to local copy of glog. For example,
  -DGLOG_SOURCE_ROOT=/path/to/extracted/copy/of/glog/tarball")
  endif()

  FetchContent_Declare(
    glog
    GIT_REPOSITORY https://github.com/google/glog
    GIT_TAG        v0.4.0
    )

  # Can't find either as a module or config. Fetch it directly.
  FetchContent_GetProperties(glog)
  if(NOT glog_POPULATED)
    FetchContent_Populate(glog)
    add_subdirectory(${glog_SOURCE_DIR} ${glog_BINARY_DIR})
  endif()
endfunction(load_glog)

find_package(glog CONFIG)
if (glog_FOUND)
  message(STATUS "Found glog CMake Config")
  set(GLOG_TARGET glog::glog)
else()
    find_package(PkgConfig)
    pkg_check_modules(glog IMPORTED_TARGET libglog)
    if (glog_FOUND)
      message(STATUS "Found glog via pkg-config")
      set(GLOG_TARGET PkgConfig::glog)
    endif()
    # This is a workaround that includes libgflags for statically
    # linked applications. The shared library libglog.so contains
    # meta-data on it's dependencies which includes libgflags
    # in particular. As a result, link failures are not a problem when
    # linking dynamically, but it is when linking statically. Seeing
    # libgflags is included on the link-line indirectly for dynamically
    # linked applications, it shouldn't be harmful to explicitly include
    # here.
    # $ readelf -d /usr/lib/x86_64-linux-gnu/libglog.so
    #   Tag        Type                         Name/Value
    #   0x0000000000000001 (NEEDED)             Shared library: [libgflags.so.2.2]
    #   0x0000000000000001 (NEEDED)             Shared library: [libunwind.so.8]
    #   0x0000000000000001 (NEEDED)             Shared library: [libpthread.so.0]
    #   0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
    #   0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
    pkg_check_modules(gflags IMPORTED_TARGET gflags)
    if (gflags_FOUND)
      message(STATUS "Found gflags via pkg-config")
      set(GFLAGS_TARGET PkgConfig::gflags)
    endif()
endif()

if (glog_FOUND)
elseif(GLOG_SOURCE_ROOT)
  message(STATUS "Using glog from ${GLOG_SOURCE_ROOT}")
  # Build glog from the provided source directory
  add_subdirectory(${GLOG_SOURCE_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/glog)
  set(GLOG_TARGET glog)
else()
  message(STATUS "Attempting to fetch glog from github")
  load_glog()
  set(GLOG_TARGET glog)
endif()
