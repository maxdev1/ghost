#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
ISO_ROOT="${ROOT}/iso"
LIMINE_VERSION="${LIMINE_VERSION:-9.2.0}"
LIMINE_DIR="${ROOT}/limine-${LIMINE_VERSION}"

if [[ $# -lt 1 ]]; then
    echo "usage: $0 pack"
    exit 1
fi

case "$1" in
    pack)
        if [[ -z "${SYSROOT:-}" ]]; then
            echo "error: SYSROOT is required" >&2
            exit 1
        fi
        if [[ -z "${TOOLCHAIN_BASE:-}" ]]; then
            echo "error: TOOLCHAIN_BASE is required" >&2
            exit 1
        fi
        if [[ -z "${KERNEL_BIN:-}" ]]; then
            echo "error: KERNEL_BIN is required" >&2
            exit 1
        fi

        echo "building ramdisk:"
        rm -rf "${ISO_ROOT}"
        mkdir -p "${ISO_ROOT}/boot/limine"
        "${TOOLCHAIN_BASE}/bin/ramdisk-writer" "${SYSROOT}" "${ISO_ROOT}/boot/ramdisk"

        echo "copying kernel and Limine artifacts"
        cp "${KERNEL_BIN}" "${ISO_ROOT}/boot/kernel"
        cp "${LIMINE_DIR}/bin/limine-bios.sys" "${ISO_ROOT}/limine-bios.sys"
        cp "${LIMINE_DIR}/bin/limine-bios.sys" "${ISO_ROOT}/boot/limine/limine-bios.sys"
        cp "${LIMINE_DIR}/bin/limine-bios-cd.bin" "${ISO_ROOT}/boot/limine/limine-bios-cd.bin"
        cp "${LIMINE_DIR}/bin/limine-uefi-cd.bin" "${ISO_ROOT}/boot/limine/limine-uefi-cd.bin"

        cat <<EOF > "${ISO_ROOT}/limine.conf"
:timeout: 5
:default: Ghost

:entry:
  name: Ghost
  path: /boot/kernel
  module_path: /boot/ramdisk
  module_string: ramdisk
  cmdline: root=/dev/ram0
EOF
        cp "${ISO_ROOT}/limine.conf" "${ISO_ROOT}/boot/limine/limine.conf"

        echo "making iso:"
        (
            cd "${ISO_ROOT}"
            xorriso -as mkisofs -R -r -J -V Ghost \
                -b boot/limine/limine-bios-cd.bin \
                -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
                -apm-block-size 2048 \
                --efi-boot boot/limine/limine-uefi-cd.bin \
                -efi-boot-part --efi-boot-image \
                --protective-msdos-label \
                "${ISO_ROOT}" -o "${ROOT}/ghost.iso"
        )

        echo "running limine BIOS installer"
        "${LIMINE_DIR}/bin/limine" bios-install "${ROOT}/ghost.iso"
        ;;
    *)
        echo "usage: $0 pack"
        exit 1
        ;;
esac
