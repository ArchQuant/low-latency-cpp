# Trading System Demos

C++ HFT Study Group | Week 7 | Chapters 9 & 10

## Prerequisites

- WSL (Ubuntu) with g++, cmake, make installed

## Build (one time)

```bash
cd Chapter10
sed -i 's/VERSION 3.0/VERSION 3.5/' CMakeLists.txt   # fix cmake version if needed
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
```
