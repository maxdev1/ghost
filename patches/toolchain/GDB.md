To create a GDB to cross-debug the running operating system (for example
in QEMU), patch the original sources with the newest patch and then build
them with:

	../gdb-7.11.1/configure --target=i686-ghost
	make
	make install