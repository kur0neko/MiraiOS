#!/bin/bash
set -e

mkdir -p "${ROOTFS_DIR}/usr/share/lxpanel/images/"

# Install the Mirai menu icon
install -m 644 files/usr/share/lxpanel/images/menu.png \
    "${ROOTFS_DIR}/usr/share/lxpanel/images/menu.png"

echo "[Mirai Menu Icon] Installed successfully."

