cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

project(buildbox-common-test CXX)

find_package(BuildboxCommon REQUIRED)

add_executable(buildbox-common-test main.cc)
target_link_libraries(buildbox-common-test Buildbox::buildbox-common PkgConfig::grpc++)
