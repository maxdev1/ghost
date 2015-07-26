**For zlib 1.2.8**
Run within source directory:

	CHOST=i686-ghost CC=i686-ghost-gcc ./configure --static --prefix=/system
	make
	make DESTDIR=/ghost/sysroot install