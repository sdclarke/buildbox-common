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

Installation
=========================

Dependencies
----------------------
buildbox-common relies on:

* gRPC
* Protobuf
* CMake
* GoogleTest
* pkg-config
* OpenSSL

GNU/Linux
---------

**Debian/Ubuntu**

Install major dependencies along with some other packages through `apt`::

    [sudo] apt-get install cmake gcc g++ googletest pkg-config libssl-dev uuid-dev

Install the `googletest`, and `googlemock` binaries ::

    cd /usr/src/googletest && mkdir build && cd build
    cmake .. && [sudo] make install


On Ubuntu, as of 18.04LTS, the package versions of `protobuf` and `grpc` are too old for use with buildbox-common. Therefore manual build and install is necessary.
The instructions below for installing the `protobuf` compiler and `grpc`  are synthesized from the `grpc source
<https://github.com/grpc/grpc/blob/master/BUILDING.md>`_.

**Last tested with `grpc` version: 1.20.0 and `protoc` version: 3.8.0**

Clone and install the latest release of `grpc` ::

    [sudo] apt-get install build-essential autoconf libtool pkg-config
    git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
    cd grpc
    git submodule update --init
    [sudo] make install

Check to see if `protoc` is installed by running `protoc --version`, if not found, install the protobuf compiler.

Inside the above `grpc` directory run::

    cd third_party/protobuf
    [sudo] make install

On Debian 10 and newer versions of Ubuntu, you can simply install the packages with `apt`::

    [sudo] apt-get install grpc++ libprotobuf-dev libgmock-dev protobuf-compiler-grpc

Compiling
--------------
Once you've installed the dependencies, you can compile `buildbox-common` using the following commands in the buildbox-common directory::

    mkdir build
    cd build
    cmake .. && [sudo] make [install]
