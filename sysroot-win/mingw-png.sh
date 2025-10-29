PREFIX=/usr/x86_64-w64-mingw32

wget https://ghostkernel.org/repository/libpng/libpng-1.6.34.tar.gz
tar xf libpng-1.6.34.tar.gz
pushd libpng-1.6.34
CC=x86_64-w64-mingw32-gcc ./configure --host=$TARGET --prefix=$PREFIX --enable-shared=no --enable-static=yes
make
sudo make install PREFIX=$PREFIX
popd
