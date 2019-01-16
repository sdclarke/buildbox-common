#!/usr/bin/env bash
set -e

cd buildbox-sample-project
mkdir -p build
cd build
cmake ..
make
./buildbox-sample-project
