	
	../freetype-2.5.3/configure --host=i686-ghost --prefix=/system --with-png=no --with-zlib=no --with-harfbuzz=no
	make
	make DESTDIR=/ghost/sysroot install
	