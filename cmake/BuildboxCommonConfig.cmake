include(CMakeFindDependencyMacro)

find_dependency(PkgConfig)
pkg_check_modules(libcrypto REQUIRED IMPORTED_TARGET libcrypto)
pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf>=3.5)
pkg_check_modules(grpc++ REQUIRED IMPORTED_TARGET grpc++>=1.10)
if(NOT APPLE)
    pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)
endif()


include("${CMAKE_CURRENT_LIST_DIR}/BuildboxCommonTargets.cmake")
