#!/bin/bash
ROOT="."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Flags
PORTS_ALL=0
LIBC_CLEAN=0
LIBC_ALL=0
LIBAPI_CLEAN=0
LIBAPI_ALL=0
APPS_ALL=0
KERNEL_CLEAN=0
KERNEL_ALL=0
APPS=()
APPS_CLEAN=0
APPS_ALL=0

EVERYTHING=1

# Define some helpers
pushd() {
	command pushd "$@" >/dev/null
}

popd() {
	command popd "$@" >/dev/null
}

build_target() {
	all_name="$@"
	print_gray "$all_name "
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

print_gray() {
	printf "\e[1;90m$1\e[0m"
}

print_skipped() {
	print_gray "skipped\n"
}

print_name() {
	printf "\e[0;7m$1\e[0m "
}

backspace_len() {
	printf "%0.s\b" $(seq 1 $@)
}

# Targets
build_ports() {
	pushd patches/ports

	print_name ports
	if [[ $PORTS_ALL == 0 && -f $SYSROOT/system/lib/libcairo.a ]]; then
		print_skipped
	else
		printf "\n"
		$SH port.sh zlib/1.2.8 | awk '$0="   "$0'
		$SH port.sh pixman/0.32.6 | awk '$0="   "$0'
		$SH port.sh libpng/1.6.18 | awk '$0="   "$0'
		$SH port.sh freetype/2.5.3 | awk '$0="   "$0'
		$SH port.sh cairo/1.12.18 | awk '$0="   "$0'
	fi

	popd
}

build_libapi() {
	pushd libapi

	print_name libapi
	if [[ $EVERYTHING == 1 || $LIBAPI_CLEAN == 1 ]]; then
		build_target clean all
		print_status
	elif [[ $LIBAPI_ALL == 1 ]]; then
		build_target all
		print_status
	else
		print_skipped
	fi

	popd
}

build_libapi_target() {
	pushd libapi

	print_name libapi
	build_target $@
	print_status

	popd
}


build_libc() {
	pushd libc

	print_name libc
	if [[ $EVERYTHING == 1 || $LIBC_CLEAN == 1 ]]; then
		build_target clean all
		print_status
	elif [[ $LIBC_ALL == 1 ]]; then
		build_target all
		print_status
	else
		print_skipped
	fi

	popd
}

build_libc_target() {
	pushd libc

	print_name libc
	build_target $@
	print_status

	popd
}


build_app() {
	pushd $1

	name="${1%/} "
	printf "  $name"
	name_back=$(backspace_len ${#name})

	build_target $2 $3
	if [ $? -eq 0 ]; then
		((apps_success = apps_success + 1))
		printf "\u2714\n"
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

	if [[ $EVERYTHING = 1 || $APPS_CLEAN = 1 || $APPS_ALL = 1 ]]; then
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
	else
		print_skipped
	fi

	popd
}

build_kernel() {
	pushd kernel

	print_name kernel
	if [[ $EVERYTHING == 1 || $KERNEL_CLEAN == 1 ]]; then
		build_target clean all
	elif [[ $KERNEL_ALL == 1 ]]; then
		build_target all
	else
		build_target repack
	fi
	print_status
	echo ""

	popd
}

print_help() {
	echo "Usage: $0"
	echo ""
	echo "  --libc             build libc"
	echo "  --libapi           build libapi"
	echo "  --kernel           build kernel"
	echo "  --ports            build ports, even if not necessary"
	echo "  --apps             build all apps"
	echo "  --apps terminal    build only listed apps (last argument)"
	echo "  --repack           only repack image"
	echo ""
	echo "By default, everything is built clean. When specifying one of the flags above,"
	echo "then only the selected modules are built and the image is repacked."
	echo ""
	echo "Adding \"-clean\" to a flag builds the target clean, like \"--libc-clean\"."
	echo ""
}

# Build script
echo ""
printf "\e[44mGhost Build\e[0m\n"
echo ""

# When running the very first time
if [ ! -f "$SYSROOT/system/lib/libgcc_s.so.1" ]; then
	echo "Running for the first time, finalize setup..."

	cp "$TOOLCHAIN_BASE/$TARGET/lib/libgcc_s.so.1" "$SYSROOT/system/lib/libgcc_s.so.1"

	mkdir "$SYSROOT/system/include"

	build_libapi_target install-headers
	build_libc_target install-headers
	build_libapi_target clean static
	build_libc_target clean static
	build_libapi_target shared
	build_libc_target shared
fi

# Parse arguments
NEXT_ARGS_APPS=0
for var in "$@"; do
	if [ $NEXT_ARGS_APPS = 1 ]; then
		APPS+=($var)
	elif [[ "$var" = "--ports" ]]; then
		PORTS_ALL=1
	elif [[ "$var" = "--apps" ]]; then
		EVERYTHING=0
		NEXT_ARGS_APPS=1
		APPS_ALL=1
	elif [[ "$var" = "--apps-clean" ]]; then
		EVERYTHING=0
		NEXT_ARGS_APPS=1
		APPS_CLEAN=1
	elif [[ "$var" = "--libapi" ]]; then
		EVERYTHING=0
		LIBAPI_ALL=1
	elif [[ "$var" = "--libapi-clean" ]]; then
		EVERYTHING=0
		LIBAPI_CLEAN=1
	elif [[ "$var" = "--libc" ]]; then
		EVERYTHING=0
		LIBC_ALL=1
	elif [[ "$var" = "--libc-clean" ]]; then
		EVERYTHING=0
		LIBC_CLEAN=1
	elif [[ "$var" = "--kernel" ]]; then
		EVERYTHING=0
		KERNEL_ALL=1
	elif [[ "$var" = "--kernel-clean" ]]; then
		EVERYTHING=0
		KERNEL_CLEAN=1
	elif [[ "$var" = "--repack" ]]; then
		EVERYTHING=0
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
