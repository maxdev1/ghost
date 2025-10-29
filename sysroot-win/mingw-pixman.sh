wget https://ghostkernel.org/repository/pixman/pixman-0.38.0.tar.gz
tar xf pixman-0.38.0.tar.gz
pushd pixman-0.38.0
PKG_CONFIG=/usr/bin/pkg-config \
PKG_CONFIG_LIBDIR=/usr/x86_64-w64-mingw32/lib/pkgconfig \
CC=x86_64-w64-mingw32-gcc ./configure --enable-static=yes --enable-shared=no --prefix=/usr/x86_64-w64-mingw32
make
sudo make install PREFIX=/usr/x86_64-w64-mingw32
popd
