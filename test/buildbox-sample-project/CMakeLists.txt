cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 14)

project(buildbox-sample-project CXX)

find_package(BuildboxCommon REQUIRED)

add_executable(buildbox-sample-project buildbox-sample-project.m.cpp)
target_link_libraries(buildbox-sample-project Buildbox::buildboxcommon PkgConfig::grpc++)
