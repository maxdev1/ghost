# ABOUT SEOS

SeOs (Sea Operating System) is a hobbyist x86-64 operating system written in C/C++ and Assembly. This project is a custom fork derived from the **Ghost operating system by Max Schlüssel (@maxdev1)**, featuring enhancements in service management and desktop aesthetics.

Main development repository by **seamiq** (GitHub: [seamiq](https://github.com/ddd800938-hue) / ddd800938-hue).

## 🌊 Key Enhancements & Features

*   **Custom Service Supervision (runit-like):** Lightweight init/service system for better process management and crash recovery.
*   **Refined GUI & Taskbar:** Improved, responsive user interface.
*   **Animated Wave Wallpaper:** Custom desktop background engine.

## Core Architecture Features
*   x86_64-based micro-kernel with SMP support.
*   `libapi` & custom C standard library (`libc`).
*   OS-specific GCC toolchain & ELF support.
*   Window server (`fenster`) & GUI toolkit.
*   Drivers for VESA/VBE, PS/2, and PCI.
*   Limine protocol compliant.

## Ported Software
*   `libpng`, `pixman`, `zlib`, `cairo`, `freetype`.
*   `musl` (for `libm` in `libc`).

## Documentation
See the `documentation` folder for technical design and build instructions.

## Live ISO
Downloadable from the releases section. Test in VirtualBox (512MB+ RAM, EFI enabled, VMSVGA graphics).

## Quick-start & Docker Compilation Warning

The build system relies on Docker containers.

> ⚠️ **Important Note on Docker:** 
> Due to regional connectivity and registry issues with Docker in Russia, the Docker-based build system **has not been personally tested by the author**. 
> 
> While scripts (`./docker-build-toolchain-image.sh` and `./build.sh`) are included, **you may encounter unexpected build errors**. Feedback and PRs regarding the Docker toolchain are welcome!

1.  Run `./docker-build-toolchain-image.sh` to build the toolchain image.
2.  Inside the container (mounted at `/seos/source`), run `./build.sh`.
3.  The `target` folder will contain the bootable ISO.

## Acknowledgements
SeOs is proudly derived from the **Ghost operating system** developed by **Max Schlüssel** (@maxdev1).
