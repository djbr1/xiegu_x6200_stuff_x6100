rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$HOME/x6200/AetherX6200Buildroot/build/host/share/buildroot/toolchainfile.cmake 
make
