PREFIX=/usr/x86_64-w64-mingw32

mkdir -p ~/temp/freetype
pushd ~/temp/freetype

wget https://ghostkernel.org/repository/freetype/freetype-2.5.3.tar.gz
tar xf freetype-2.5.3.tar.gz
pushd freetype-2.5.3
mkdir build
pushd build

CC=x86_64-w64-mingw32-gcc ../configure --prefix=$PREFIX --with-harfbuzz=no --enable-shared=no --enable-static=yes
make
sudo make install PREFIX=$PREFIX

popd
popd
popd

rm -rf ~/temp/freetype
