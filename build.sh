#!/bin/bash
ROOT="."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Flags
BUILD_LIBC_CLEAN=1
BUILD_LIBC=0
BUILD_LIBAPI=1
BUILD_PORTS=0
KERNEL_REPACK_ONLY=0
KERNEL_BUILD_WITH_APPS=0
APPS=()
APPS_CLEAN=0

# Define some helpers
pushd() {
	command pushd "$@" >/dev/null
}

popd() {
	command popd "$@" >/dev/null
}

build_target() {
	$SH build.sh $@ >ghost-build.log 2>&1
}

print_status() {
	if [ $? -eq 0 ]; then
		printf "\e[1;92m\u2714\e[0m\n"
	else
		printf "\e[1;31m\u274c\e[0m log: "
		printf "\n\n"
		tail -n 100 ghost-build.log | awk '$0="   "$0'
		printf "\n"
	fi
}

print_skipped() {
	printf "\e[1;90mskipped\e[0m\n"
}

print_name() {
	printf "\e[0;7m$1\e[0m "
}

backspace_len() {
	printf "%0.s\b" $(seq 1 $@)
}

# Targets
build_ports() {
	print_name ports
	if [[ $BUILD_PORTS = 0 && -f $SYSROOT/system/lib/libcairo.a ]]; then
		print_skipped
	else
		printf "\n"
		pushd patches/ports
		$SH port.sh zlib/1.2.8 | awk '$0="   "$0'
		$SH port.sh pixman/0.32.6 | awk '$0="   "$0'
		$SH port.sh libpng/1.6.18 | awk '$0="   "$0'
		$SH port.sh freetype/2.5.3 | awk '$0="   "$0'
		$SH port.sh cairo/1.12.18 | awk '$0="   "$0'
		popd
	fi
}

build_libapi() {
	pushd libapi
	print_name libapi
	if [ $BUILD_LIBAPI = 1 ]; then
		build_target clean && build_target all
		print_status
	else
		print_skipped
	fi
	popd
}

build_libc() {
	pushd libc
	print_name libc
	if [ $BUILD_LIBC_CLEAN = 1 ]; then
		build_target clean && build_target all
		print_status
	elif [ $BUILD_LIBC = 1 ]; then
		build_target all
		print_status
	else
		print_skipped
	fi
	popd
}

build_app() {
	pushd $1
	name="${1%/}"
	printf "  $name"
	name_back=$(backspace_len ${#name})

	build_target $2 $3
	if [ $? -eq 0 ]; then
		((apps_success = apps_success + 1))
		printf " \u2714\n"
	else
		printf $name_back
		printf "\e[1;31m$name\e[0m \u274c log: "
		printf "\n\n"
		tail -n 100 ghost-build.log | awk '$0="   "$0'
		printf "\n"
	fi
	((apps_total = apps_total + 1))
	popd
}

build_apps() {
	pushd applications
	print_name applications
	apps_success=0
	apps_total=0

	printf "\n"
	NUM_APPS=${#APPS[@]}
	if [ $NUM_APPS -gt "0" ]; then
		for var in ${APPS[@]}; do
			if [ -d $var ]; then
				if [ $APPS_CLEAN = 1 ]; then
					build_app $var clean all
				else
					build_app $var all
				fi
			else
				printf "\e[1;31m$var (?)\e[0m "
			fi
		done
	else
		for dir in */; do
			build_app $dir clean all
		done
	fi
	echo "  ($apps_success/$apps_total successful)"
	popd
}

build_kernel() {
	print_name kernel
	pushd kernel
	if [ $KERNEL_BUILD_WITH_APPS = 1 ]; then
		build_target all
	elif [ $KERNEL_REPACK_ONLY = 1 ]; then
		build_target repack
	else
		build_target clean && build_target all
	fi
	print_status
	popd
	echo ""
}

print_help() {
	echo "Usage: $0"
	echo "  --apps                  build all apps clean and repack the image"
	echo "  --apps windowserver     build only specific apps and repack the image"
	echo "  --apps-clean terminal   build only specific apps clean and repack the image"
	echo "  --kernel                build kernel and not repack only"
	echo "  --ports                 enables building all ports, even if not necessary"
	echo ""
}

# Build script
echo ""
printf "\e[44mGhost Build\e[0m\n"
echo ""

COLLECT_APPS=0
for var in "$@"; do
	if [ $COLLECT_APPS = 1 ]; then
		APPS+=($var)
	elif [[ "$var" = "--apps" ]]; then
		BUILD_LIBC_CLEAN=0
		BUILD_LIBAPI=0
		KERNEL_REPACK_ONLY=1
		COLLECT_APPS=1
	elif [[ "$var" = "--apps-clean" ]]; then
		BUILD_LIBC_CLEAN=0
		BUILD_LIBAPI=0
		KERNEL_REPACK_ONLY=1
		COLLECT_APPS=1
		APPS_CLEAN=1
	elif [[ "$var" = "--ports" ]]; then
		BUILD_PORTS=1
	elif [[ "$var" = "--kernel" ]]; then
		KERNEL_BUILD_WITH_APPS=1
	elif [[ "$var" = "--libc" ]]; then
		BUILD_LIBC=1
	elif [[ "$var" = "--help" || "$var" = "-h" || "$var" = "?" ]]; then
		print_help
		exit 0
	fi
done

build_ports
build_libapi
build_libc
build_apps
build_kernel
