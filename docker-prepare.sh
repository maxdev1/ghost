#!/bin/bash
#
# This script is executed within the Docker container when building the image
#
set -e

# Don't ask questions
export DEBIAN_FRONTEND=noninteractive

# Set env for required autoconf version
export AUTOCONF=autoconf2.69

# Install requirements
apt-get update
apt-get install -y \
    libmpfr-dev libgmp-dev libmpc-dev \
    autoconf2.69 pkg-config xorriso grub-pc-bin \
    make texinfo flex bison gcc g++ nasm \
    asciidoc asciidoctor \
    patch curl

# Clean up
rm -rf /var/lib/apt/lists/*

# Do actual toolchain build
cd /ghost/source
./toolchain.sh
cd /

# Clean up
rm -rf /tmp/*
