FROM debian:buster

WORKDIR /app

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    googletest \
    grpc++ \
    libbenchmark-dev \
    libssl-dev \
    pkg-config \
    uuid-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ARG EXTRA_CMAKE_FLAGS=
ARG BUILD_TESTS=OFF

RUN cd /usr/src/googletest \
    && mkdir build && cd build && cmake .. && make install && make clean

COPY . /buildbox-common

RUN cd /buildbox-common && mkdir build && cd build && cmake -DBUILD_TESTING=${BUILD_TESTS} "${EXTRA_CMAKE_FLAGS}" .. && make -j$(nproc) install
