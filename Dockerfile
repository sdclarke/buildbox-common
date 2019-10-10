FROM debian:buster AS build

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
    && mkdir build && cd build && cmake .. && make install && rm -rf /usr/src/googletest

COPY . /buildbox-common

RUN cd /buildbox-common && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_TESTING=ON .. && make all -j$(nproc) && make install DESTDIR=/buildbox-common/install

FROM debian:buster AS test

WORKDIR /app

RUN apt-get update && apt-get install -y make cmake && apt-get clean

COPY --from=build /buildbox-common /buildbox-common

# The tests need the runtimes
RUN apt-get update && apt-get install -y make grpc++ && apt-get clean

RUN cd /buildbox-common/build && make test

FROM build AS compile_sample_project

RUN cd /buildbox-common/build &&  make install && cd /buildbox-common/test/ && ./compile_sample_project.sh

FROM debian:buster AS release

COPY --from=build /buildbox-common/install /
