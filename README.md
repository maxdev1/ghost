# ABOUT GHOST
**Ghost** is an operating system for the Intel x86 platform written
from scratch in C++ and assembly. It aspires to follow a pure
micro kernel design and do stuff as clean as possible. Some of the
important features are:

- multiprocessor & multithreading support
- window server, GUI with homemade toolkit
- patched GCC, OS specific toolchain
- custom libc implementation
- libstdc++ port
- extensive kernel API library (libapi)
- userspace C++ library (libuser)
- PS/2 keyboard & mouse driver, VESA video driver
- userspace filesystem driver support
- ELF binary support
- ipc mechanisms: pipes, signals, messages, shared memory
- named processes
- serial COM1 kernel logging
- virtual 8086 for BIOS calls

## Status
This is the bleeding edge source version of August 28, 2015.

- Idle task uses "HLT" to prevent 100% processor usage
- stdio is fixed, piping works
- Directory reading functionality is reworked
- Spawner and others uses variable messages
- Color in the terminal, and a fancy start screen
- Minor bugfixes here and there
- `g_atomic_wait` is now `g_atomic_lock`
- `g_spawn` has slightly changed, its now possible to specify
  in and out descriptors of a process when spawning it

TODOs:
- Directory reading causes crashes
- Removing the old recv_msg/send_msg interface
- Milestone: window server rewrite

## Structure
- `applications`
	sources for the essential system applications
- `documentation`
	about kernel concepts and features
- `kernel`
	sources of the kernel
- `libapi`
	sources of the userspace API library
- `libc`
	sources of the Ghost C library implementation
- `libuser`
	sources of the C++ userspace library
- `patches`
	patches & instructions for ports and toolchain setup
- `tools`
	set of tools that are used for development

There is an additional repository, the `ghost-sysroot` that is used as the
base for the filesystem image for a blank installation.

## Building Ghost
See the `patches/toolchain/TOOLCHAIN.md` for instructions on how to set up the
toolchain.

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com
	
-Max Schlüssel
