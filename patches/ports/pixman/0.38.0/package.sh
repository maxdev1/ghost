REMOTE_ARCHIVE="https://ghostkernel.org/repository/pixman/pixman-0.38.0.tar.gz"
UNPACKED_DIR=pixman-0.38.0
ARCHIVE=pixman-0.38.0.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=1

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	export PKG_CONFIG=$TARGET-pkg-config.sh
	./configure --host=$TARGET --prefix=$PREFIX
	make
	make DESTDIR=$SYSROOT install
}
