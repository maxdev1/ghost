# Ghost OS

**Ghost** is an x86-64 operating system written in C, C++ and assembly, with a
microkernel, userspace drivers and the ƒenster desktop.

[Project website](https://ghostkernel.org/) ·
[Documentation](https://ghostkernel.org/documentation/) ·
[ISO releases](https://github.com/maxdev1/ghost/releases)

This project is independently developed and not based on Linux, BSD, or any other existing OS.

## Build and run

The [build guide](documentation/general/building.adoc) covers the cross-toolchain
and component targets. For the Docker environment, run from the repository root:

```sh
git submodule update --init
./docker-build-toolchain-image.sh
```

Inside the container, `./build.sh --ci` produces `target/ghost.iso`. Boot it with
an x86-64 VM, for example:

```sh
qemu-system-x86_64 -cdrom target/ghost.iso -m 1024 -smp 2
```

The [installer](applications/install/README.md) copies the live system to an
empty AHCI disk for persistent FAT-backed boot.

## Components

- SMP kernel with ELF/shared-library loading, tasks, IPC and memory management.
- `libapi` syscall wrappers, a C runtime and an `x86_64-ghost` GCC toolchain.
- PCI, ACPI, PS/2, AHCI, FAT, boot-framebuffer and VMSVGA services.
- [ƒenster](applications/fenster/README.md) window server/UI toolkit, desktop,
  terminal, shell, editor, file browser and [Doom](applications/doom/README.md).
- Ported Cairo, FreeType, Pixman, libpng, zlib and musl mathematical routines.

![Screenshot of 0.12.0](https://ghostkernel.org/files/ghost-0.22.2.png)

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com

-Max Schlüssel

