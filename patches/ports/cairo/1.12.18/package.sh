REMOTE_ARCHIVE="https://ghostkernel.org/repository/cairo/cairo-1.12.18.tar.xz"
UNPACKED_DIR=cairo-1.12.18
ARCHIVE=cairo-1.12.18.tar.xz

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
if [ -z "$PKG_CONFIG" ]; then
	export PKG_CONFIG=$TARGET-pkg-config.sh
fi
	CFLAGS="-DCAIRO_NO_MUTEX=1" ../$UNPACKED_DIR/configure --host=$TARGET --prefix=$PREFIX --enable-xlib=no --enable-shared=yes --enable-static=no
	make -j8
	make DESTDIR=$SYSROOT install
}
