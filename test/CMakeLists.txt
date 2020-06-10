include(${CMAKE_SOURCE_DIR}/cmake/BuildboxGTestSetup.cmake)

# Uncomment and add any additional srcs/headers containing code for tests to link against as an additional argument to the GLOB (e.g fixtures).
# Then add "extra_libs" to the target_link_libraries in the macro
# file(GLOB EXTRA_LIBS_SRCS fixtures/*)
# add_library(extra_libs ${EXTRA_LIBS_SRCS})
# target_include_directories(extra_libs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
# target_link_libraries(extra_libs buildboxcommon ${GMOCK_TARGET} ${GTEST_TARGET})

# This macro creates the specific test executable, google test and google mock.
# Current limitation is the TEST_SOURCE must only be one file.
# If more than 1 file is needed, combine the sources into a list.
# The current working directory can be modified by passing in an extra arg.
macro(add_buildboxcommon_test TEST_NAME TEST_SOURCE)
    # Create a separate test executable per test source.
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # This allows us to pass an optional argument if the cwd for the test is not the default.
    set(ExtraMacroArgs ${ARGN})
    list(LENGTH ExtraMacroArgs NumExtraArgs)
    if(${NumExtraArgs} GREATER 0)
      list(GET ExtraMacroArgs 0 TEST_WORKING_DIRECTORY)
    else()
      set(TEST_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})
    target_link_libraries(${TEST_NAME} PUBLIC buildboxcommon ${GTEST_MAIN_TARGET} ${GMOCK_TARGET})
endmacro()

add_buildboxcommon_test(client_test buildboxcommon_client.t.cpp)
add_buildboxcommon_test(stageddirectory_tests buildboxcommon_stageddirectory.t.cpp)
add_buildboxcommon_test(localcasstageddirectory_tests buildboxcommon_localcasstageddirectory.t.cpp)
add_buildboxcommon_test(connectionoptions_tests buildboxcommon_connectionoptions.t.cpp)
add_buildboxcommon_test(fileutils_tests buildboxcommon_fileutils.t.cpp)
add_buildboxcommon_test(timeutils_tests buildboxcommon_timeutils.t.cpp)
add_buildboxcommon_test(systemutils_tests buildboxcommon_systemutils.t.cpp)
add_buildboxcommon_test(logging_tests buildboxcommon_logging.t.cpp)
add_buildboxcommon_test(runner_tests buildboxcommon_runner.t.cpp)
add_buildboxcommon_test(temporarydirectory_tests buildboxcommon_temporarydirectory.t.cpp)
add_buildboxcommon_test(temporaryfile_tests buildboxcommon_temporaryfile.t.cpp)
add_buildboxcommon_test(grpcretry_tests buildboxcommon_grpcretry.t.cpp)
add_buildboxcommon_test(protos_tests buildboxcommon_protos.t.cpp)
add_buildboxcommon_test(requestmetadata_tests buildboxcommon_requestmetadata.t.cpp)
add_buildboxcommon_test(mergeutil_tests buildboxcommon_mergeutil.t.cpp)
add_buildboxcommon_test(direntwrapper_tests buildboxcommon_direntwrapper.t.cpp)
add_buildboxcommon_test(metrics_durationmetrictimer_test buildboxcommonmetrics_durationmetrictimer_test.t.cpp)
add_buildboxcommon_test(metrics_durationmetricvalue_test buildboxcommonmetrics_durationmetricvalue_test.t.cpp)
add_buildboxcommon_test(metrics_countingmetricvalue_test buildboxcommonmetrics_countingmetricvalue_test.t.cpp)
add_buildboxcommon_test(metrics_countingmetricutil_test buildboxcommonmetrics_countingmetricutil_test.t.cpp)
add_buildboxcommon_test(metrics_countingmetric_test buildboxcommonmetrics_countingmetric_test.t.cpp)
add_buildboxcommon_test(metrics_gaugemetric_test buildboxcommonmetrics_gaugemetric.t.cpp)
add_buildboxcommon_test(metrics_gaugemetricvalue_test buildboxcommonmetrics_gaugemetricvalue.t.cpp)
add_buildboxcommon_test(metrics_gaugemetricutil_test buildboxcommonmetrics_gaugemetricutil.t.cpp)
add_buildboxcommon_test(metrics_metriccollector_test buildboxcommonmetrics_metriccollector_test.t.cpp)
add_buildboxcommon_test(metrics_metriccollectorfactory_one_test buildboxcommonmetrics_metriccollectorfactory_one_test.t.cpp)
add_buildboxcommon_test(metrics_metriccollectorfactory_two_test buildboxcommonmetrics_metriccollectorfactory_two_test.t.cpp)
add_buildboxcommon_test(metrics_metricguard_disabled_test buildboxcommonmetrics_metricguard_disabled_test.t.cpp)
add_buildboxcommon_test(metrics_metricguard_enabled_test buildboxcommonmetrics_metricguard_enabled_test.t.cpp)
add_buildboxcommon_test(metrics_publisherguard_test buildboxcommonmetrics_publisherguard_test.t.cpp)
add_buildboxcommon_test(metrics_scopedperiodicpublisherdaemon_test buildboxcommonmetrics_scopedperiodicpublisherdaemon_test.t.cpp)
add_buildboxcommon_test(metrics_statsdpublisher_test_stderr buildboxcommonmetrics_statsdpublisher_test_stderr.t.cpp)
add_buildboxcommon_test(metrics_statsdpublisher_test_file buildboxcommonmetrics_statsdpublisher_test_file.t.cpp)
add_buildboxcommon_test(metrics_totaldurationmetrictimer_test buildboxcommonmetrics_totaldurationmetrictimer_test.t.cpp)
add_buildboxcommon_test(metrics_totaldurationmetricvalue_test buildboxcommonmetrics_totaldurationmetricvalue_test.t.cpp)
add_buildboxcommon_test(metrics_metricsconfigurator_test buildboxcommonmetrics_metricsconfigurator.t.cpp)
add_buildboxcommon_test(metrics_statsdpublishercreator_test buildboxcommonmetrics_statsdpublishercreator.t.cpp)
add_buildboxcommon_test(metrics_metricsconfigutil_test buildboxcommonmetrics_metricsconfigutil.t.cpp)
add_buildboxcommon_test(metrics_metricsconfigurator_statsd_test buildboxcommonmetrics_metricsconfigurator_statsd.t.cpp)
add_buildboxcommon_test(metrics_testingutils_test buildboxcommonmetrics_testingutils.t.cpp)
add_buildboxcommon_test(metrics_udpwriter_test buildboxcommonmetrics_udpwriter_test.t.cpp)
add_buildboxcommon_test(commandline_test buildboxcommon_commandline.t.cpp)

# These tests use a different cwd.
add_buildboxcommon_test(cashash_tests buildboxcommon_cashash.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/)
add_buildboxcommon_test(fallbackstageddirectory_tests buildboxcommon_fallbackstageddirectory.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/buildboxcommon_fallbackstageddirectory)
add_buildboxcommon_test(merklize_tests buildboxcommon_merklize.t.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/merklize)
