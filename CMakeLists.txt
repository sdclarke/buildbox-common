cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

project(buildboxcommon VERSION 0.0.1 LANGUAGES CXX)

## REQUIRED FOR AIX
# gcc on AIX can't deal with -isystem that contains C++ .h files
if(${CMAKE_SYSTEM_NAME} MATCHES "AIX" AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)
endif()

find_package(OpenSSL REQUIRED)
set(OPENSSL_TARGET OpenSSL::Crypto)

find_package(Protobuf)
if(Protobuf_FOUND)
    if(Protobuf_VERSION VERSION_LESS 3.5)
        message(FATAL_ERROR "Version of Protobuf too old (${Protobuf_VERSION}), should be >=3.5.")
    elseif(NOT TARGET protobuf::libprotobuf)
        message(AUTHOR_WARNING "Cmake too old to define protobuf::libprotobuf, will try PkgConfig instead.")
    else()
        set(PROTOBUF_TARGET protobuf::libprotobuf)
        set(PROTOBUF_INCLUDE_DIRS ${Protobuf_INCLUDE_DIRS})
    endif()
endif()

if(NOT DEFINED PROTOBUF_TARGET)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf>=3.5)
    set(PROTOBUF_TARGET PkgConfig::protobuf)
    set(PROTOBUF_INCLUDE_DIRS ${protobuf_INCLUDE_DIRS})
endif()

# Try to find gRPC with find_package and fall back to pkg_config
find_package(gRPC)
if(gRPC_FOUND)
    set(GRPC_TARGET gRPC::grpc++)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(grpc++ REQUIRED IMPORTED_TARGET grpc++>=1.10)
    set(GRPC_TARGET PkgConfig::grpc++)
endif()
add_subdirectory(protos)

file(GLOB BUILDBOXCOMMON_CPP "buildboxcommon/*.cpp")

set_source_files_properties(${PROTO_GENERATED_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-all -Wno-error -Wno-extra -Wno-conversion")

add_library(buildboxcommon STATIC
    ${BUILDBOXCOMMON_CPP}
    ${PROTO_GENERATED_SRCS})

## Only add flags when compiling buildbox-common, not tests.
## PRIVATE specifies don't add these flags to libraries that link against this.
if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    set(DEBUG_FLAGS -Werror -Wextra -pedantic-errors -Wall -Wconversion -Wno-vla)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    target_compile_options(buildboxcommon PRIVATE -Werror=inconsistent-missing-override -Werror=shadow ${DEBUG_FLAGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    # We want to avoid setting errors on GNU + AIX because system headers + not being able to use -isystem
    if(${CMAKE_SYSTEM_NAME} MATCHES "AIX")
        message("Skipping all warnings due to GNU compiler + AIX system")
    else()
        target_compile_options(buildboxcommon PRIVATE -Wall -Werror=suggest-override -Werror=shadow ${DEBUG_FLAGS})
    endif()
else()
    target_compile_options(buildboxcommon PRIVATE -Wall ${DEBUG_FLAGS})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "AIX")
    # AIX's sys/sysmacros.h defines a macro called major(). Bazel's semver
    # protobuf defines a property called major, which produces a method called
    # major() in the generated code.

    # To resolve this, pass a flag to the compiler to prevent sys/sysmacros.h
    # from defining anything.
    target_compile_options(buildboxcommon PUBLIC -D_H_SYSMACROS)
endif()

target_link_libraries(buildboxcommon ${PROTOBUF_TARGET} ${GRPC_TARGET} ${OPENSSL_TARGET})

if(NOT APPLE)
    # macOS includes UUID generation functionality in libc, but on other platforms
    # it's a separate library.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)
    target_link_libraries(buildboxcommon PkgConfig::uuid)
endif()

target_include_directories(buildboxcommon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buildboxcommon)

# gcc on AIX can't deal with -isystem that contains C++ .h files
if(${CMAKE_SYSTEM_NAME} MATCHES "AIX" AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    target_include_directories(buildboxcommon PRIVATE ${PROTO_GEN_DIR})
else()
    target_include_directories(buildboxcommon SYSTEM PRIVATE ${PROTO_GEN_DIR})
endif()

set_source_files_properties(${PROTO_GENERATED_SRCS} PROPERTIES GENERATED 1)
add_dependencies(buildboxcommon generate_protobufs)

install(
    TARGETS buildboxcommon EXPORT BuildboxCommonTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include/buildbox
)

# Hash function used to calculate Digests.
# Integer value as defined in `enum DigestFunction.Value` in the REAPI proto
# (`protos/build/bazel/remote/execution/v2/remote_execution.proto`).
if(NOT BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE)
    set(BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE 1) # By default, use SHA-256
    message("`BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE` not set, "
            "setting `DigestFunction.Value` to default value: 1 (SHA-256)")
else()
    message("Setting `DigestFunction.Value` to "
            "${BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE}")
endif()

target_compile_definitions(buildboxcommon PUBLIC BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE=${BUILDBOXCOMMON_DIGEST_FUNCTION_VALUE})

file(GLOB BUILDBOXCOMMON_H "buildboxcommon/*.h")

install(FILES ${BUILDBOXCOMMON_H} DESTINATION include/buildbox)
install(DIRECTORY ${PROTO_GEN_DIR}/google DESTINATION include/buildbox FILES_MATCHING PATTERN "*.h*")
install(DIRECTORY ${PROTO_SOURCE_DIR}/google DESTINATION include/buildbox FILES_MATCHING PATTERN "*.proto")
install(DIRECTORY ${PROTO_GEN_DIR}/build DESTINATION include/buildbox FILES_MATCHING PATTERN "*.h*")
install(DIRECTORY ${PROTO_SOURCE_DIR}/build DESTINATION include/buildbox FILES_MATCHING PATTERN "*.proto")

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
