rm -rf build/

cd third/src

cd ffmpeg
make clean
./configure --prefix=../../build/linux --enable-shared --disable-programs --enable-libx264 --enable-gpl \
  --enable-libfdk-aac --enable-nonfree --enable-libx265
make -j
make install