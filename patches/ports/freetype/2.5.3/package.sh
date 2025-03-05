REMOTE_ARCHIVE="https://ghostkernel.org/repository/freetype/freetype-2.5.3.tar.gz"
UNPACKED_DIR=freetype-2.5.3
ARCHIVE=freetype-2.5.3.tar.gz

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	CFLAGS="-fPIC" ../$UNPACKED_DIR/configure --host=$TARGET --prefix=$PREFIX --with-harfbuzz=no --enable-shared=yes --enable-static=no
	make -j8
	make DESTDIR=$SYSROOT install
}
