#!/bin/bash

cd ../

mkdir install

git clone https://github.com/awslabs/aws-c-common.git
cd aws-c-common
mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX=../../install ../
if [[ $? -ne 0 ]] ; then
    exit 1
fi

make install
if [[ $? -ne 0 ]] ; then
    exit 1
fi

cd ../..

cd aws-c-compression
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=../../install $@ ../
if [[ $? -ne 0 ]] ; then
    exit 1
fi

make
if [[ $? -ne 0 ]] ; then
    exit 1
fi

LSAN_OPTIONS=verbosity=1:log_threads=1 ctest --output-on-failure
if [[ $? -ne 0 ]] ; then
    exit 1
fi

cd ..

./cppcheck.sh ../install/include
if [[ $? -ne 0 ]] ; then
    exit 1
fi
