** Install fake pkg-config **
	
	export PKG_CONFIG=/ghost/bin/i686-ghost-pkg-config.sh
	../pixman-0.32.6/configure --host=i686-ghost --prefix=/system
	make
	make DESTDIR=/ghost/sysroot install
	