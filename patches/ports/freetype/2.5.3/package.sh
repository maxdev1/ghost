REMOTE_ARCHIVE="https://ghostkernel.org/repository/freetype/freetype-2.5.3.tar.gz"
UNPACKED_DIR=freetype-2.5.3
ARCHIVE=freetype-2.5.3.tar.gz

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	../$UNPACKED_DIR/configure --host=$HOST --prefix=$PREFIX --with-harfbuzz=no
	make
	make DESTDIR=$SYSROOT install
}
