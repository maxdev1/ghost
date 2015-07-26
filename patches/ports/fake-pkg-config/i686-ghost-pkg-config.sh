#!/bin/sh
# Fill these in appropriately:
export PKG_CONFIG_SYSROOT_DIR=/ghost/sysroot
export PKG_CONFIG_LIBDIR=/ghost/sysroot/system/lib/pkgconfig
# TODO: If it works this should probably just be set to the empty string.
export PKG_CONFIG_PATH=$PKG_CONFIG_LIBDIR
# Use --static here if your OS only has static linking.
# TODO: Perhaps it's a bug in the libraries if their pkg-config files doesn't
#       record that only static libraries were built.
exec pkg-config --static "$@"