REMOTE_ARCHIVE="https://ghostkernel.org/repository/cairo/cairo-1.12.18.tar.xz"
UNPACKED_DIR=cairo-1.12.18
ARCHIVE=cairo-1.12.18.tar.xz

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	export PKG_CONFIG=i686-ghost-pkg-config.sh
	CFLAGS="-DCAIRO_NO_MUTEX=1" ../$UNPACKED_DIR/configure --host=$HOST --prefix=$PREFIX --enable-xlib=no
	make
	make DESTDIR=$SYSROOT install
}