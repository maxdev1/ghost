PREFIX=/usr/x86_64-w64-mingw32

mkdir -p ~/temp/cairo
pushd ~/temp/cairo

wget https://ghostkernel.org/repository/cairo/cairo-1.12.18.tar.xz
tar xf cairo-1.12.18.tar.xz
pushd cairo-1.12.18

mkdir build
pushd build
CC=x86_64-w64-mingw32-gcc \
CFLAGS="-O2 -D_FORTIFY_SOURCE=0 -I/usr/x86_64-w64-mingw32/include" \
CPPFLAGS="-D_FORTIFY_SOURCE=0 -I/usr/x86_64-w64-mingw32/include" \
LDFLAGS="-L/usr/x86_64-w64-mingw32/lib" \
PKG_CONFIG_LIBDIR=/usr/x86_64-w64-mingw32/lib/pkgconfig \
PKG_CONFIG_PATH=/usr/x86_64-w64-mingw32/lib/pkgconfig \
../configure --host=x86_64-w64-mingw32 \
  --prefix=$PREFIX \
  --enable-win32=yes \
  --enable-ft=yes \
  --disable-xlib
make -j8
sudo make install PREFIX=$PREFIX
popd

popd

popd

rm -rf ~/temp/cairo
