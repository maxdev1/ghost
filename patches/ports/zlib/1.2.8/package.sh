REMOTE_ARCHIVE="https://ghostkernel.org/repository/zlib/zlib-1.2.8.tar.gz"
UNPACKED_DIR=zlib-1.2.8
ARCHIVE=zlib-1.2.8.tar.gz
REQUIRES_INSTALL_IN_SOURCE_DIR=0

port_unpack() {
	tar -xf $ARCHIVE
}

port_install() {
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_TOOLCHAIN_FILE=../../../../../../i686-ghost-toolchain.cmake ../$UNPACKED_DIR
	make
	make DESTDIR=$SYSROOT install
}
