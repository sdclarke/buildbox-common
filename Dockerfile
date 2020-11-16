FROM debian:buster

WORKDIR /app

ARG EXTRA_CMAKE_FLAGS=

RUN apt-get update && apt-get install -y \
    attr \
    cmake \
    gcc \
    g++ \
    git \
    googletest \
    grpc++ \
    libbenchmark-dev \
    libgoogle-glog-dev \
    libssl-dev \
    pkg-config \
    uuid-dev \
    && apt-get clean \
    && cd /usr/src/googletest \
    && mkdir build && cd build && cmake .. && make install

COPY . /buildbox-common

RUN cd /buildbox-common && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_TESTING=OFF "${EXTRA_CMAKE_FLAGS}" .. && make install
