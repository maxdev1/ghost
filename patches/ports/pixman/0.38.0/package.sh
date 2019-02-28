REMOTE_ARCHIVE="https://ghostkernel.org/repository/pixman/pixman-0.38.0.tar.gz"
UNPACKED_DIR=pixman-0.38.0
ARCHIVE=pixman-0.38.0.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=1

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	export PKG_CONFIG=i686-ghost-pkg-config.sh
	./configure --host=$HOST --prefix=$PREFIX
	make
	make DESTDIR=$SYSROOT install
}
