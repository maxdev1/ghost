#!/usr/bin/env bash
set -euo pipefail

installAssetsUsage()
{
	printf 'Usage: %s --root DIRECTORY --kernel FILE --limine DIRECTORY\n' "$0"
}

assetRoot=
kernelFile=
limineDirectory=
while (($#)); do
	case "$1" in
		--root|--kernel|--limine)
			if (($# < 2)) || [[ -z "$2" || "$2" == --* ]]; then
				printf 'Missing value for %s\n' "$1" >&2
				exit 2
			fi
			case "$1" in
				--root) assetRoot=$2 ;;
				--kernel) kernelFile=$2 ;;
				--limine) limineDirectory=$2 ;;
			esac
			shift 2
			;;
		-h|--help) installAssetsUsage; exit 0 ;;
		*) printf 'Unknown argument: %s\n' "$1" >&2; exit 2 ;;
	esac
done
if [[ -z "$assetRoot" || -z "$kernelFile" || -z "$limineDirectory" ]]; then
	installAssetsUsage >&2
	exit 2
fi

modules=(
	applications/pcidriver.bin
	applications/acpidriver.bin
	applications/devicemanager.bin
	applications/ahcidriver.bin
	applications/storagemanager.bin
	applications/fatdriver.bin
	applications/efifbdriver.bin
	applications/vmsvgadriver.bin
	system/lib/libpci.so
	system/lib/libacpi.so
	system/lib/libdevice.so
	system/lib/libblockdevice.so
	system/lib/libstorage.so
	system/lib/libgcc_s.so.1
	system/lib/libvideo.so
	system/lib/libghostapi.so
	system/lib/libc.so
	system/lib/apstartup.o
)

mkdir -p "$assetRoot/boot/limine" "$assetRoot/system/install"
cp "$kernelFile" "$assetRoot/boot/kernel"
cp "$limineDirectory/limine-bios.sys" "$assetRoot/boot/limine/limine-bios.sys"
for module in "${modules[@]}"; do
	if [[ ! -f "$assetRoot/$module" ]]; then
		printf 'Missing bootstrap file: %s\n' "$module" >&2
		exit 1
	fi
done
{
	printf 'timeout:0\n\n/Ghost\n    protocol: limine\n    path: boot():/boot/kernel\n    cmdline: root=fat:00000000:00000800\n'
	printf '    module_path: boot():/%s\n' "${modules[@]}"
} > "$assetRoot/boot/limine/limine.conf"

temporaryDirectory=$(mktemp -d "${TMPDIR:-/tmp}/ghost-boot.XXXXXX")
trap 'rm -rf "$temporaryDirectory"' EXIT
image="$temporaryDirectory/boot.img"

#
# Create a 64 MiB MBR template with an active FAT32-LBA partition starting at
# sector 2048 and spanning 129024 sectors. Limine uses the preceding boot gap.
#
dd if=/dev/zero of="$image" bs=512 count=1 seek=131071 2>/dev/null
printf '\x80\xfe\xff\xff\x0c\xfe\xff\xff\x00\x08\x00\x00\x00\xf8\x01\x00' |
	dd of="$image" bs=1 seek=446 conv=notrunc 2>/dev/null
printf '\x55\xaa' | dd of="$image" bs=1 seek=510 conv=notrunc 2>/dev/null
"$limineDirectory/limine" bios-install "$image"
dd if="$image" of="$assetRoot/system/install/limine.bin" bs=512 count=128 2>/dev/null
