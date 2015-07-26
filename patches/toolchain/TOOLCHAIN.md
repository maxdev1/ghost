# How to set up the toolchain #

### 0. If on Mac OS ###
Export the correct GNU GCC:

	export CC=gcc-4.9
	export CXX=g++-4.9
	export CPP=cpp-4.9
	export LD=gcc-4.9
	
### 1. Preparation ###
Check out the "ghost" repository, make $REPO refer to this location.
Check out the "ghost-sysroot" repository to "/ghost/sysroot".
Unpack binutils 2.24 and GCC 4.9.1 to your working directory,
make $WORK refers to this location.

	export REPO=/path/to/your/ghost
	export WORK=/workdir

*Binutils*
1. Unpack binutils
2. Patch binutils
3. Run automake 1.11.1 in the "ld" directory

*GCC*
1. Unpack GCC
2. Patch GCC
3. Run autoconf 2.64 in the "libstdc++-v3" directory


### 2. Install Ghosts libc & api headers ###
Build the "changes" tool:
	
	cd $REPO/tools/changes
	sh build.sh all

Install the headers of libc and api:

	cd $REPO/libc
	sh build.sh install-headers
	cd $REPO/api
	sh build.sh install-headers

The headers have now been installed into the sysroot at `/ghost/sysroot`


### 3. Building binutils & gcc ###

Add "/ghost/bin" to your PATH variable.

Export the required settings:

	export TARGET=i686-ghost
	export PREFIX=/ghost
	export SYSROOT=$PREFIX/sysroot

Configuring and building binutils:

	cd $WORK
	mkdir build-binutils
	cd build-binutils
	../binutils-2.24/configure --target=$TARGET --prefix=$PREFIX --disable-nls --disable-werror --with-sysroot=$SYSROOT
	make all && make install

Configuring and building GCC:

	cd $WORK
	mkdir build-gcc
	cd build-gcc
	../gcc-4.9.1/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --with-sysroot=$SYSROOT
	make all-gcc
	make install-gcc
	make all-target-libgcc
	make install-target-libgcc

Build libc:
	
	cd $REPO/libc
	sh build.sh all
	
Build the API:
	
	cd $REPO/api
	sh build.sh all

Build libstdc++-v3:
	
	cd $WORK/build-gcc
	make all-target-libstdc++-v3
	make install-target-libstdc++-v3
