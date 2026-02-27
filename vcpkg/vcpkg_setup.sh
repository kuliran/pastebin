#!/bin/bash

set -e

REPO_ROOT=${1:-/app}

VCPKG_ROOT="/opt/vcpkg"
export PATH="$VCPKG_ROOT:$PATH"
export VCPKG_FORCE_SYSTEM_BINARIES=1
export CC=clang
export CXX=clang++

mkdir -p $VCPKG_DEFAULT_BINARY_CACHE
mkdir -p $VCPKG_INSTALLED_DIR
sudo chown -R $(id -u):$(id -g) $REPO_ROOT/vcpkg/

cp $REPO_ROOT/vcpkg/triplets/* $VCPKG_ROOT/triplets/
