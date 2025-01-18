#!/bin/bash
#
# The purpose of this script is to create the ghost-toolchain Docker image
#
set -e

echo "Building toolchain within Docker container..."
docker run --name ghost-toolchain-setup -v "$(pwd):/ghost/source" ubuntu:latest /ghost/source/docker-prepare.sh >>ghost-build.log 2>&1

echo "Committing image..."
docker commit --change='WORKDIR /ghost/source' --change='CMD ["/bin/bash"]' ghost-toolchain-setup ghost-toolchain

echo "Removing temporary container..."
docker rm ghost-toolchain-setup

echo "Starting toolchain container now!"
echo "To build the operating system, run within container:   ./build.sh"
echo "After exiting, you can join it again with:             docker exec -it ghost-toolchain bash"
docker run -it --name ghost-toolchain -v "$(pwd):/ghost/source" ghost-toolchain
