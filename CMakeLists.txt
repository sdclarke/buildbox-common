cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

project(buildboxcommon VERSION 0.0.1 LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcrypto REQUIRED IMPORTED_TARGET libcrypto)
pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf>=3.5)
pkg_check_modules(grpc++ REQUIRED IMPORTED_TARGET grpc++>=1.10)
add_subdirectory(protos)

add_library(buildboxcommon STATIC
    buildboxcommon/buildboxcommon_client.cpp
    ${PROTO_GENERATED_SRCS})

target_link_libraries(buildboxcommon PkgConfig::protobuf PkgConfig::grpc++)
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
install(FILES
    buildboxcommon/buildboxcommon_client.h
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
          "${CMAKE_CURRENT_BINARY_DIR}/BuildboxCommon/BuildboxCommonConfigVersion.cmake"
    DESTINATION lib/cmake/BuildboxCommon
)

