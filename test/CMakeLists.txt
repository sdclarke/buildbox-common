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

add_executable(stageddirectory_tests
  buildboxcommon_stageddirectory.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(stageddirectory_tests buildboxcommon ${GMOCK_TARGET})
add_test(
    NAME stageddirectory_tests
    COMMAND stageddirectory_tests
)

add_executable(fallbackstageddirectory_tests
  buildboxcommon_fallbackstageddirectory.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(fallbackstageddirectory_tests buildboxcommon ${GMOCK_TARGET})
add_test(
    NAME fallbackstageddirectory_tests
    COMMAND fallbackstageddirectory_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/buildboxcommon_fallbackstageddirectory
)

add_executable(localcasstageddirectory_tests
  buildboxcommon_localcasstageddirectory.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(localcasstageddirectory_tests buildboxcommon ${GMOCK_TARGET})
add_test(
    NAME localcasstageddirectory_tests
    COMMAND localcasstageddirectory_tests
)

add_executable(connectionoptions_tests buildboxcommon_connectionoptions.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(connectionoptions_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME connectionoptions_tests COMMAND connectionoptions_tests)

add_executable(fileutils_tests
  buildboxcommon_fileutils.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(fileutils_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME fileutils_tests COMMAND fileutils_tests)

add_executable(timeutils_tests
  buildboxcommon_timeutils.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(timeutils_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME timeutils_tests COMMAND timeutils_tests)

add_executable(systemutils_tests
  buildboxcommon_systemutils.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(systemutils_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME systemutils_tests COMMAND systemutils_tests)

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

add_executable(mergeutil_tests
  buildboxcommon_mergeutil.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(mergeutil_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME mergeutil_tests COMMAND mergeutil_tests)

add_executable(direntwrapper_tests
  buildboxcommon_direntwrapper.t.cpp
  buildboxcommon_tests.m.cpp)
target_link_libraries(direntwrapper_tests buildboxcommon ${GTEST_TARGET})
add_test(NAME direntwrapper_tests COMMAND direntwrapper_tests)

add_executable(metrics_durationmetrictimer_test buildboxcommonmetrics_durationmetrictimer_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_durationmetrictimer_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_durationmetrictimer_test COMMAND metrics_durationmetrictimer_test)

add_executable(metrics_durationmetricvalue_test buildboxcommonmetrics_durationmetricvalue_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_durationmetricvalue_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_durationmetricvalue_test COMMAND metrics_durationmetricvalue_test)

add_executable(metrics_countingmetricvalue_test buildboxcommonmetrics_countingmetricvalue_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_countingmetricvalue_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_countingmetricvalue_test COMMAND metrics_countingmetricvalue_test)

add_executable(metrics_countingmetricutil_test buildboxcommonmetrics_countingmetricutil_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_countingmetricutil_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_countingmetricutil_test COMMAND metrics_countingmetricutil_test)

add_executable(metrics_countingmetric_test buildboxcommonmetrics_countingmetric_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_countingmetric_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_countingmetric_test COMMAND metrics_countingmetric_test)

add_executable(metrics_metriccollector_test buildboxcommonmetrics_metriccollector_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_metriccollector_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_metriccollector_test COMMAND metrics_metriccollector_test)

add_executable(metrics_metriccollectorfactory_one_test buildboxcommonmetrics_metriccollectorfactory_one_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_metriccollectorfactory_one_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_metriccollectorfactory_one_test COMMAND metrics_metriccollectorfactory_one_test)

add_executable(metrics_metriccollectorfactory_two_test buildboxcommonmetrics_metriccollectorfactory_two_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_metriccollectorfactory_two_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_metriccollectorfactory_two_test COMMAND metrics_metriccollectorfactory_two_test)

add_executable(metrics_metricguard_disabled_test buildboxcommonmetrics_metricguard_disabled_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_metricguard_disabled_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_metricguard_disabled_test COMMAND metrics_metricguard_disabled_test)

add_executable(metrics_metricguard_enabled_test buildboxcommonmetrics_metricguard_enabled_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_metricguard_enabled_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_metricguard_enabled_test COMMAND metrics_metricguard_enabled_test)

add_executable(metrics_publisherguard_test buildboxcommonmetrics_publisherguard_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_publisherguard_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_publisherguard_test COMMAND metrics_publisherguard_test)

add_executable(metrics_statsdpublisher_test_stderr buildboxcommonmetrics_statsdpublisher_test_stderr.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_statsdpublisher_test_stderr ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_statsdpublisher_test_stderr COMMAND metrics_statsdpublisher_test_stderr)

add_executable(metrics_statsdpublisher_test_file buildboxcommonmetrics_statsdpublisher_test_file.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_statsdpublisher_test_file ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_statsdpublisher_test_file COMMAND metrics_statsdpublisher_test_file)


add_executable(metrics_totaldurationmetrictimer_test buildboxcommonmetrics_totaldurationmetrictimer_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_totaldurationmetrictimer_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_totaldurationmetrictimer_test COMMAND metrics_totaldurationmetrictimer_test)

add_executable(metrics_totaldurationmetricvalue_test buildboxcommonmetrics_totaldurationmetricvalue_test.t.cpp buildboxcommon_tests.m.cpp)
target_link_libraries(metrics_totaldurationmetricvalue_test ${GTEST_TARGET} buildboxcommon)
add_test(NAME metrics_totaldurationmetricvalue_test COMMAND metrics_totaldurationmetricvalue_test)
