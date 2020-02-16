REMOTE_ARCHIVE="https://ghostkernel.org/repository/zlib/zlib-1.2.8.tar.gz"
UNPACKED_DIR=zlib-1.2.8
ARCHIVE=zlib-1.2.8.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=1

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	CHOST=$HOST CC=$HOST-gcc ./configure --prefix=$PREFIX
	make
	make DESTDIR=$SYSROOT install
}