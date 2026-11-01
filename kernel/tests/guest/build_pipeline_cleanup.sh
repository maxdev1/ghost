#!/usr/bin/env bash
set -eo pipefail
source ./ghost.sh
mkdir -p kernel/tests/bin/guest-pipeline
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/pipeline_cleanup.cpp \
    -o kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.bin

test_root=$(mktemp -d /tmp/ghost-pipeline-iso.XXXXXX)
trap 'rm -rf "$test_root"' EXIT
cp -a sysroot "$test_root/sysroot"
cp kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.bin "$test_root/sysroot/applications/ahcidriver.bin"
mkdir -p "$test_root/iso/boot"
cp -a target/iso/boot/limine "$test_root/iso/boot/limine"
cp kernel/kernel "$test_root/iso/boot/kernel"
ramdisk-writer "$test_root/sysroot" "$test_root/iso/boot/ramdisk"
test_iso="$PWD/kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.iso"
xorriso -as mkisofs -R -r -J -b /boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
    --efi-boot /boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
    --protective-msdos-label "$test_root/iso" -o "$test_iso"
"./target/limine-$LIMINE_VERSION/bin/limine" bios-install "$test_iso"
