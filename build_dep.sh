cd code
rm -rf third/build/linux

cd DreamMediaNative
rm -rf build

cmake -B build -DCMAKE_INSTALL_PREFIX=../third/build/linux
cmake --build build -j
cmake --install build --config Release