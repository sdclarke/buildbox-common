What is buildbox-common?
========================

``buildbox-common`` is a library containing code used by multiple parts of
BuildBox. Its API is unstable and it should not be used by applications
other than BuildBox.

Currently, ``buildbox-common`` contains all the Protocol Buffer definitions
used by BuildBox, as well as code to connect to and interact with
Content-Addressable Storage servers.

Writing tests for dependent projects
------------------------------------

``buildbox-common`` ships a file called ``BuildboxGTestSetup.cmake``, which sets
up variables to simplify writing tests. You can include it with ::

    include(${CMAKE_INSTALL_PREFIX}/lib/cmake/BuildboxCommon/BuildboxGTestSetup.cmake)
    
And it provides the ``GMOCK_TARGET`` and ``GTEST_TARGET`` variables.

See `buildbox-run-hosttools's test/CMakeLists.txt <https://gitlab.com/BuildGrid/buildbox/buildbox-run-hosttools/blob/master/test/CMakeLists.txt>`_
for a usage example.