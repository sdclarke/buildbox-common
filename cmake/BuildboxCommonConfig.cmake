include(CMakeFindDependencyMacro)
find_package(OpenSSL REQUIRED)
set(OPENSSL_TARGET OpenSSL::Crypto)
find_dependency(PkgConfig)
pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf>=3.5)

find_package(gRPC)
if(grpc_FOUND)
    set(GRPC_TARGET gRPC::grpc++)
else()
    pkg_check_modules(grpc++ REQUIRED IMPORTED_TARGET grpc++>=1.10)
    set(GRPC_TARGET PkgConfig::grpc++)
endif()

if(NOT APPLE)
    pkg_check_modules(uuid REQUIRED IMPORTED_TARGET uuid)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/BuildboxCommonTargets.cmake")
