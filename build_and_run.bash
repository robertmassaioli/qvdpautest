#!/bin/bash

BUILD_DIR="localbuild"

if [ ! -d "$BUILD_DIR" ]
then
   mkdir "$BUILD_DIR"
fi
cd "$BUILD_DIR"

if ! which cmake > /dev/null
then
   echo "Could not find an install of 'cmake' in the current Environment PATH. Please install CMake and put it in the path."
   exit 1
fi

cmake ..
make
./src/qvdpautest ../data
