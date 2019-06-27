include(${CMAKE_SOURCE_DIR}/cmake/BuildboxGTestSetup.cmake)

add_executable(cashash_tests buildboxcommon_cashash.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(cashash_tests buildboxcommon ${GTEST_TARGET})
add_test(
    NAME cashash_tests
    COMMAND cashash_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
)

add_executable(client_tests
  buildboxcommon_client.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(client_tests buildboxcommon ${GMOCK_TARGET})
add_test(NAME client_tests COMMAND client_tests)

add_executable(fallbackstageddirectory_tests
  buildboxcommon_fallbackstageddirectory.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(fallbackstageddirectory_tests buildboxcommon ${GMOCK_TARGET})
add_test(
    NAME fallbackstageddirectory_tests
    COMMAND fallbackstageddirectory_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/buildboxcommon_fallbackstageddirectory
)

add_executable(connectionoptions_tests buildboxcommon_connectionoptions.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(connectionoptions_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME connectionoptions_tests COMMAND connectionoptions_tests)

add_executable(fileutils_tests
  buildboxcommon_fileutils.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(fileutils_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME fileutils_tests COMMAND fileutils_tests)

add_executable(logging_tests buildboxcommon_logging.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(logging_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME logging_tests COMMAND logging_tests)

add_executable(runner_tests buildboxcommon_runner.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(runner_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME runner_tests COMMAND runner_tests)

add_executable(temporarydirectory_tests
  buildboxcommon_temporarydirectory.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(temporarydirectory_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME temporarydirectory_tests COMMAND temporarydirectory_tests)

add_executable(temporaryfile_tests
  buildboxcommon_temporaryfile.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(temporaryfile_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME temporaryfile_tests COMMAND temporaryfile_tests)

add_executable(grpcretry_tests
  buildboxcommon_grpcretry.t.cpp
  buildboxcommon_tests.m.cpp)

target_link_libraries(grpcretry_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME grpcretry_tests COMMAND grpcretry_tests)
add_executable(merklize_tests
  buildboxcommon_merklize.t.cpp
  buildboxcommon_tests.m.cpp)

target_link_libraries(merklize_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME merklize_tests
        COMMAND merklize_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/merklize)

add_executable(protos_tests
               buildboxcommon_protos.t.cpp
               buildboxcommon_tests.m.cpp)
target_link_libraries(protos_tests buildboxcommon ${GMOCK_TARGET})
add_test(NAME protos_tests COMMAND protos_tests)

add_executable(requestmetadata_tests
  buildboxcommon_requestmetadata.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(requestmetadata_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME requestmetadata_tests COMMAND requestmetadata_tests)
