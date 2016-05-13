REMOTE_ARCHIVE="https://ghostkernel.org/repository/pixman/pixman-0.32.6.tar.gz"
UNPACKED_DIR=pixman-0.32.6
ARCHIVE=pixman-0.32.6.tar.gz

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	export PKG_CONFIG=i686-ghost-pkg-config.sh
	../$UNPACKED_DIR/configure --host=$HOST --prefix=$PREFIX
	make
	make DESTDIR=$SYSROOT install
}