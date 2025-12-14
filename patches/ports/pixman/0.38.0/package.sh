REMOTE_ARCHIVE="https://ghostkernel.org/repository/pixman/pixman-0.38.0.tar.gz"
UNPACKED_DIR=pixman-0.38.0
ARCHIVE=pixman-0.38.0.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=1

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
if [ -z "$PKG_CONFIG" ]; then
	export PKG_CONFIG=$TARGET-pkg-config.sh
fi
	./configure --host=$TARGET --prefix=$PREFIX --enable-shared=yes --enable-static=no
	make -C pixman -j8
	make -C pixman DESTDIR=$SYSROOT install
	mkdir -p "$SYSROOT${PREFIX}/lib/pkgconfig"
	cat <<EOF > "$SYSROOT${PREFIX}/lib/pkgconfig/pixman-1.pc"
prefix=${PREFIX}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include/pixman-1

Name: pixman-1
Description: Pixman pixel manipulation library
Version: 0.38.0
Libs: -L\${libdir} -lpixman-1
Cflags: -I\${includedir}
EOF
}
