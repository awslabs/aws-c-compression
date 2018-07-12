#!/bin/bash

cd ../

mkdir install

git clone https://github.com/awslabs/aws-c-common.git
cd aws-c-common
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../../install ../
make install
cd ../..

cd aws-c-compression
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=../../install ../
make
make test
cd ..
./cppcheck.sh ../install/include
