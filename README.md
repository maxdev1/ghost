# ABOUT GHOST
**Ghost** is a hobbyist operating system for the x86-64 platform.
The OS is written from scratch in C/C++ and Assembly.

This is the main development repository.

**Project website:** http://ghostkernel.org/

## Documentation
See the `documentation` folder. It contains information about the technical design as well as detailed building instructions.

A usually up-to-date version of the documentation is provided here: https://ghostkernel.org/documentation/

## Live ISO

If you just want to play around with the latest state, you can download an ISO
image from the [release section](https://github.com/maxdev1/ghost/releases).

The suggested way to test it is in VirtualBox with at least 512MB of RAM and the
VMSVGA graphics adapter enabled for better performance.

## Quick-start

1. On your host machine (where Docker is installed), run `./docker-build-toolchain-image.sh`. This will 
    build an image that contains the cross-compiler and other tools required for building the operating system.
2. Once the process has finished, it will open a bash within the container. The container has this repository folder mounted to `/ghost/source`.
3. Build the operating system by running `./build.sh` in that directory within the container.

Afterwards, the `target` folder will contain the bootable ISO image.

### Bootstrapping the Ghost toolchain locally

Outside the Docker workflow you can build the cross-toolchain directly on your host by running:

```bash
./bootstrap-toolchain.sh
```

The script wraps the `cmake -S cmake/ghost-toolchain-bootstrap ...` invocation, produces the toolchain/sysroot under `build-ghost/`, and prints the `-DTOOLCHAIN_BASE` and `-DSYSROOT` paths you should pass to your main CMake configure step.

If your distribution ships Autoconf newer than 2.69, run `./tools/install-autoconf-269.sh` once and prepend the printed `.../autoconf-2.69/bin` to `PATH` before invoking the toolchain scripts—the Ghost toolchain currently requires Autoconf 2.69 exactly.

## Features
* x86_64-based micro-kernel
* SMP multi-processor support
* Comprehensive kernel interaction library (libapi)
* Own C standard library (libc)
* OS-specific GCC toolchain
* ELF binary & shared library support
* Window server & toolkit
* Support for C++ in kernel & userland
* Various interprocess-communication methods
* Drivers for
  * VESA/VBE video output
  * PS/2 keyboard & mouse
  * PCI handling
* Limine protocol compliance
* It's also very cool

The OS uses the [fenster windowserver](https://github.com/maxdev1/fenster) which originated from this project:

![Screenshot of 0.12.0](https://ghostkernel.org/files/ghost-0.22.2.png)

## Ported software
* libpng
* pixman
* zlib
* cairo
* freetype
* musl (provides libm part of Ghost libc)

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com
	
-Max Schlüssel
