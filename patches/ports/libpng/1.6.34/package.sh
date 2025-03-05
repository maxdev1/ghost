REMOTE_ARCHIVE="https://ghostkernel.org/repository/libpng/libpng-1.6.18.tar.gz"
UNPACKED_DIR=libpng-1.6.18
ARCHIVE=libpng-1.6.18.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=1

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	./configure --host=$TARGET --prefix=$PREFIX --enable-shared=yes --enable-static=no
	make -j8
	make DESTDIR=$SYSROOT install
}