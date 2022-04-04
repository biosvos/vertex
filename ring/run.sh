#!/bin/bash

if [ -f CMakeLists.txt ]; then
	mkdir -p build && cd build && cmake .. && make -j$(nproc)
	cd ..
else
	echo "CMakeLists.txt is not exist"
fi

./build/test_ring
