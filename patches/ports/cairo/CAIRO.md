1. Build zlib
2. Build pixman
3. Build libpng
4. Use fake pkg-config
5. Apply patch, then (from separate build directory):
	
	export PKG_CONFIG=i686-ghost-pkg-config.sh
	CFLAGS="-DCAIRO_NO_MUTEX=1" ../cairo-1.12.18/configure --host=i686-ghost --prefix=/system --enable-xlib=no
	make
	make DESTDIR=/ghost/sysroot install