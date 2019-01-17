FROM debian:buster

WORKDIR /app

RUN apt-get update && apt-get install -y \
    attr \
    cmake \
    gcc \
    g++ \
    git \
    googletest \
    grpc++ \
    libssl-dev \
    pkg-config \
    uuid-dev \
    && apt-get clean \
    && cd /usr/src/googletest \
    && mkdir build && cd build && cmake .. && make && make install

COPY . /buildbox-common

RUN cd /buildbox-common && mkdir build && cd build && cmake -DBUILD_TESTING=OFF .. && make && make install
