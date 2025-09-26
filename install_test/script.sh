#!/bin/bash

build_dir=$1

[[ ! -d "$build_dir" ]] && { echo "invalid build dir: $1"; exit 1; }

echo "== klib install test =="

cd ./install_test || exit 1

cmake --install ../$build_dir --config=Debug --prefix=./packages || exit 1
cmake --install ../$build_dir --config=Release --prefix=./packages || exit 1

cp -f ../CMakePresets.json ./exe
cmake -S exe --preset=default -B=build -DCMAKE_INSTALL_PREFIX=./packages || exit 1

cmake --build build --config=Debug || exit 1
build/Debug/exe || exit 1

cmake --build build --config=Release || exit 1
build/Release/exe || exit 1
