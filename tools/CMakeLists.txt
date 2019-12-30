add_executable(casdownload buildboxcommon_casdownload.m.cpp)
target_include_directories(casdownload PUBLIC ${CMAKE_SOURCE_DIR}/buildboxcommon ${PROTO_GEN_DIR})
target_link_libraries(casdownload buildboxcommon ${PROTOBUF_TARGET})
