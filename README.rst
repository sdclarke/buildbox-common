What is buildbox-common?
========================

``buildbox-common`` is a library containing code used by multiple parts of
BuildBox and `recc <https://gitlab.com/bloomberg/recc>`_. Its API is unstable
and it should not be used by other applications.

Currently, ``buildbox-common`` contains all the Protocol Buffer definitions
used by BuildBox, as well as code to connect to and interact with
Content-Addressable Storage servers.

It also contains a metrics library ``buildboxcommonmetrics``, the details of
which can be found in it's own dedicated `README <buildbox-common/buildboxcommonmetrics/README.md>`_.

Writing tests for dependent projects
------------------------------------

``buildbox-common`` ships a file called ``BuildboxGTestSetup.cmake``, which sets
up variables to simplify writing tests. You can include it with ::

    include(${CMAKE_INSTALL_PREFIX}/lib/cmake/BuildboxCommon/BuildboxGTestSetup.cmake)

And it provides the ``GMOCK_TARGET`` and ``GTEST_TARGET`` variables.

See `buildbox-run-hosttools's test/CMakeLists.txt <https://gitlab.com/BuildGrid/buildbox/buildbox-run-hosttools/blob/master/test/CMakeLists.txt>`_
for a usage example.

Installation
=========================

Dependencies
----------------------
buildbox-common relies on:

* gRPC
* Protobuf
* CMake
* GoogleTest
* glog
* pkg-config
* OpenSSL

GNU/Linux
---------

**Debian/Ubuntu**

Install major dependencies along with some other packages through `apt`::

    [sudo] apt-get install cmake g++ gcc grpc++ googletest libgmock-dev libgoogle-glog-dev libprotobuf-dev libssl-dev pkg-config protobuf-compiler-grpc uuid-dev

On Ubuntu, as of 18.04LTS, the package versions of `protobuf` and `grpc` are too old for use with buildbox-common. Therefore manual build and install is necessary.
Please follow the `upstream instructions to install gprc and protobuf <https://github.com/grpc/grpc/blob/master/BUILDING.md>`_.

Compiling
--------------
Once you've installed the dependencies, you can compile `buildbox-common` using the following commands in the buildbox-common directory::

    mkdir build
    cd build
    cmake .. && [sudo] make [install]
