cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

project(buildboxcommon VERSION 0.0.1 LANGUAGES CXX)

## REQUIRED FOR AIX
# gcc on AIX can't deal with -isystem that contains C++ .h files
if(${CMAKE_SYSTEM_NAME} MATCHES "AIX" AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcrypto REQUIRED IMPORTED_TARGET libcrypto)
pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf>=3.5)

# Try to find gRPC with find_package and fall back to pkg_config
find_package(gRPC)
if(gRPC_FOUND)
    set(GRPC_TARGET gRPC::grpc++)
else()
    pkg_check_modules(grpc++ REQUIRED IMPORTED_TARGET grpc++>=1.10)
    set(GRPC_TARGET PkgConfig::grpc++)
endif()
add_subdirectory(protos)

file(GLOB BUILDBOXCOMMON_CPP "buildboxcommon/*.cpp")

add_library(buildboxcommon STATIC
    ${BUILDBOXCOMMON_CPP}
    ${PROTO_GENERATED_SRCS})

## Only add flags when compiling casd, not tests. PRIVATE specifies don't add these flags to libraries that link against this.
if(CMAKE_CXX_COMPILER_ID EQUAL Clang)
    target_compile_options(buildboxcommon PRIVATE -Wall -Werror=inconsistent-missing-override)
elseif(CMAKE_CXX_COMPILER_ID EQUAL GNU)
    target_compile_options(buildboxcommon PRIVATE -Wall -Werror=suggest-override)
else()
    target_compile_options(buildboxcommon PRIVATE -Wall)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "AIX")
    # AIX's sys/sysmacros.h defines a macro called major(). Bazel's semver
    # protobuf defines a property called major, which produces a method called
    # major() in the generated code.

    # To resolve this, pass a flag to the compiler to prevent sys/sysmacros.h
    # from defining anything.
    target_compile_options(buildboxcommon PUBLIC -D_H_SYSMACROS)
endif()

target_link_libraries(buildboxcommon PkgConfig::protobuf ${GRPC_TARGET})
target_link_libraries(buildboxcommon PkgConfig::libcrypto)
if(NOT APPLE)
    # macOS includes UUID generation functionality in libc, but on other platforms
    # it's a separate library.
    pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)
    target_link_libraries(buildboxcommon PkgConfig::uuid)
endif()

target_include_directories(buildboxcommon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buildboxcommon)
target_include_directories(buildboxcommon PRIVATE ${PROTO_GEN_DIR})

set_source_files_properties(${PROTO_GENERATED_SRCS} PROPERTIES GENERATED 1)
add_dependencies(buildboxcommon generate_protobufs)

install(
    TARGETS buildboxcommon EXPORT BuildboxCommonTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include/buildbox
)

file(GLOB BUILDBOXCOMMON_H "buildboxcommon/*.h")

install(FILES
    ${BUILDBOXCOMMON_H}
    DESTINATION include/buildbox)
install(DIRECTORY ${PROTO_GEN_DIR}/google DESTINATION include/buildbox FILES_MATCHING PATTERN "*.h*")
install(DIRECTORY ${PROTO_GEN_DIR}/build DESTINATION include/buildbox FILES_MATCHING PATTERN "*.h*")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/BuildboxCommon/BuildboxCommonConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
export(EXPORT BuildboxCommonTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/BuildboxCommon/BuildboxCommonTargets.cmake"
    NAMESPACE Buildbox::
)
configure_file(cmake/BuildboxCommonConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/BuildboxCommon/BuildboxCommonConfig.cmake"
  COPYONLY
)
install(
    EXPORT BuildboxCommonTargets
    FILE BuildboxCommonTargets.cmake
    NAMESPACE Buildbox::
    DESTINATION lib/cmake/BuildboxCommon
)
install(
    FILES cmake/BuildboxCommonConfig.cmake
          cmake/BuildboxGTestSetup.cmake
          "${CMAKE_CURRENT_BINARY_DIR}/BuildboxCommon/BuildboxCommonConfigVersion.cmake"
    DESTINATION lib/cmake/BuildboxCommon
)

include(CTest)
if(BUILD_TESTING)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/buildboxcommon ${PROTO_GEN_DIR})
    include_directories(third_party/grpc/include)
    add_subdirectory(test)
endif()
