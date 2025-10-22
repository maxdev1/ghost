#!/bin/bash
ROOT="."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Prioritized applications that need to be built first
APPLICATION_PRIORITY=("libproperties" "libdevice" "libps2" "libps2driver" "libinput" "libwindow" "libfont" "libterminal" "libvideo" "libpci" "libahci")

# Flags
FIRST_RUN=0
CI_BUILD=0

PORTS_ALL=0
LIBC_CLEAN=0
LIBC_ALL=0
LIBAPI_CLEAN=0
LIBAPI_ALL=0
KERNEL_CLEAN=0
KERNEL_ALL=0
APPS=()
APPS_CLEAN=0
APPS_ALL=0

EVERYTHING=1

# Targets
requireTool mtools

# Define some helpers
pushd() {
	command pushd "$@" >/dev/null
}

popd() {
	command popd "$@" >/dev/null
}

move_to_front() {
	local array_name=$1
	local item=$2
	local new_array=("$item")
	local i
	eval "for i in \"\${$array_name[@]}\"; do
		if [[ \"\$i\" != \"$item\" ]]; then
			new_array+=(\"\$i\")
		fi
	done"
	eval "$array_name=(\"\${new_array[@]}\")"
}

build_target() {
	all_name="$@"
	print_gray "$all_name "
	$SH build.sh $@ >ghost-build.log 2>&1
}

print_status() {
	if [ $? -eq 0 ]; then
		printf "\e[1;92m✓\e[0m "
    if [[ $CI_BUILD == 1 ]]; then
      printf "\n\n"
      tail -n 10 ghost-build.log | awk '$0="   "$0'
    fi
    printf "\n"
	else
		printf "\e[1;31m❌\e[0m "
		printf "\n\n"
		tail -n 100 ghost-build.log | awk '$0="   "$0'
		printf "\n"

    if [[ $CI_BUILD == 1 ]]; then
      exit 1
    fi
	fi
}

print_gray() {
	printf "\e[1;90m$1\e[0m"
}

print_name() {
	printf "\e[0;7m$1\e[0m "
}

backspace_len() {
	printf "%0.s\b" $(seq 1 $@)
}

# Build limine if required
verify_limine() {
  pushd target

  # TODO: Maybe move this all to toolchain setup
  if [ ! -d "limine-$LIMINE_VERSION" ]; then
    print_name "limine-prepare"
    printf "\n"
    curl "$LIMINE_SOURCE" -k -o "limine-$LIMINE_VERSION.tar.gz"
    tar -xf "limine-$LIMINE_VERSION.tar.gz"
    pushd "limine-$LIMINE_VERSION"
    ./configure --enable-bios-cd --enable-uefi-cd
    make
    popd
  fi

  mkdir -p $SYSROOT/system/include
  cp "limine-$LIMINE_VERSION/limine.h" "$SYSROOT/system/include/limine.h"

  popd
}

build_ports() {
	pushd patches/ports

	if [[ $PORTS_ALL != 0 || ! -f $SYSROOT/system/lib/libcairo.so ]]; then
	  print_name ports
		printf "\n"
		$SH port.sh zlib/1.2.8 | awk '$0="   "$0'
		$SH port.sh pixman/0.38.0 | awk '$0="   "$0'
		$SH port.sh libpng/1.6.34 | awk '$0="   "$0'
		$SH port.sh freetype/2.5.3 | awk '$0="   "$0'
		$SH port.sh cairo/1.12.18 | awk '$0="   "$0'
	fi

	popd
}

build_libapi() {
	pushd libapi

	if [[ $EVERYTHING == 1 || $LIBAPI_CLEAN == 1 ]]; then
	  print_name libapi
		build_target clean all
		print_status
	elif [[ $LIBAPI_ALL == 1 ]]; then
	  print_name libapi
		build_target all
		print_status
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

	if [[ $EVERYTHING == 1 || $LIBC_CLEAN == 1 ]]; then
	  print_name libc
		build_target clean all
		print_status
	elif [[ $LIBC_ALL == 1 ]]; then
	  print_name libc
		build_target all
		print_status
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

	if [[ $apps_header_printed != 1 ]]; then
	  apps_header_printed=1
	  print_name applications
	  printf "\n"
	fi

	name="${1%/} "
	printf "  $name"
	name_back=$(backspace_len ${#name})

	build_target $2 $3
	if [ $? -eq 0 ]; then
		((apps_success = apps_success + 1))
		printf "✓\n"
	else
		printf $name_back
		printf "\e[1;31m$name\e[0m ❌ log: "
		printf "\n\n"
		tail -n 100 ghost-build.log | awk '$0="   "$0'
		printf "\n"
	fi
	((apps_total = apps_total + 1))

	popd
}

build_apps() {
	pushd applications

	apps_success=0
	apps_total=0

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
			# When building everything, we need to sort by priority first
			mapfile -t app_dirs < <(find . -maxdepth 1 -type d -not -name "." | sed 's|^\./||')

			for ((i=${#APPLICATION_PRIORITY[@]}-1; i>=0; i--)); do
				move_to_front app_dirs "${APPLICATION_PRIORITY[i]}"
			done

			for dir in ${app_dirs[@]}; do
				if [[ $APPS_CLEAN = 1 || $EVERYTHING = 1 ]]; then
					build_app $dir clean all
				else
					build_app $dir all
				fi
			done
		fi
		echo "  ($apps_success/$apps_total successful)"
	fi

	popd
}

build_kernel() {
	pushd kernel

	if [[ $EVERYTHING == 1 || $KERNEL_CLEAN == 1 ]]; then
	  print_name kernel
		build_target clean all
	  print_status
	elif [[ $KERNEL_ALL == 1 ]]; then
	  print_name kernel
		build_target all
	  print_status
	fi

	popd
}

build_pack() {
  pushd target

	print_name pack
  build_target pack
  print_status

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
	echo "  --pack             pack & build image"
	echo ""
	echo "By default, everything is built clean. When specifying one of the flags above,"
	echo "then only the selected modules are built and the image is packed."
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
  FIRST_RUN=1
fi

# Always check limine
verify_limine

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
		APPS_CLEAN=0
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
	elif [[ "$var" = "--pack" ]]; then
		EVERYTHING=0
	elif [[ "$var" = "--ci" ]]; then
	  CI_BUILD=1
	elif [[ "$var" = "--help" || "$var" = "-h" || "$var" = "?" ]]; then
		print_help
		exit 0
	fi
done

# On first run
if [[ $FIRST_RUN = 1 ]]; then
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

# Run build steps
build_ports
build_libapi
build_libc
build_apps
build_kernel
build_pack
printf "\n"
