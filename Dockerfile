FROM debian:buster

WORKDIR /app

RUN apt-get update && apt-get install -y \
    attr \
    cmake \
    gcc \
    g++ \
    git \
    grpc++ \
    libssl-dev \
    pkg-config \
    uuid-dev \
    && apt-get clean

COPY . /buildbox-common

RUN cd /buildbox-common && mkdir build && cd build && cmake .. && make && make install
