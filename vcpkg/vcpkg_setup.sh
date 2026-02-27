#!/bin/bash

set -e

VCPKG_ROOT="${VCPKG_ROOT:-/opt/vcpkg}"
VCPKG_MANIFEST_DIR="${VCPKG_MANIFEST_DIR:-/app/vcpkg}"
VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-/opt/vcpkg_cache}"
VCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR:-/opt/vcpkg_installed}"
export PATH="$VCPKG_ROOT:$PATH"
export VCPKG_FORCE_SYSTEM_BINARIES=1
export CC=clang
export CXX=clang++

echo "running vcpkg_setup..."
whoami
stat $VCPKG_ROOT

mkdir -p $VCPKG_DEFAULT_BINARY_CACHE
mkdir -p $VCPKG_INSTALLED_DIR
chown -R $(id -u):$(id -g) $VCPKG_MANIFEST_DIR

echo "copying from $VCPKG_MANIFEST_DIR/triplets"
cp $VCPKG_MANIFEST_DIR/triplets/* $VCPKG_ROOT/triplets/