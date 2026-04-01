#!/usr/bin/env bash

set -e

OPENSSL_VERSION="3.0.13"
PREFIX="/opt/musl-openssl"

echo "[+] Building OpenSSL $OPENSSL_VERSION with musl..."

# Download if not exists
if [ ! -f "openssl-$OPENSSL_VERSION.tar.gz" ]; then
    echo "[+] Downloading OpenSSL..."
    wget https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz
fi

# Extract
rm -rf openssl-$OPENSSL_VERSION
tar -xvf openssl-$OPENSSL_VERSION.tar.gz

cd openssl-$OPENSSL_VERSION

# Configure
echo "[+] Configuring..."
CC=musl-gcc ./Configure linux-x86_64 no-shared no-secure-memory no-engine --prefix=$PREFIX

# Build
echo "[+] Building..."
make -j$(nproc)

# Install
echo "[+] Installing to $PREFIX..."
sudo make install

echo "[+] Done!"
echo "[+] Libraries installed at: $PREFIX/lib64"
