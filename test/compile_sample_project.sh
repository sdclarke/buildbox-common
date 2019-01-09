#!/usr/bin/env bash
set -e

cd sample_project
mkdir -p build
cd build
cmake ..
make
./buildbox-common-test
