PREFIX=/usr/x86_64-w64-mingw32

wget https://ghostkernel.org/repository/zlib/zlib-1.2.8.tar.gz
tar xf zlib-1.2.8.tar.gz
pushd zlib-1.2.8
CC=x86_64-w64-mingw32-gcc ./configure --static --prefix=$PREFIX
make
sudo make install PREFIX=$PREFIX
popd
