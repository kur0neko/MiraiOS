#!/bin/bash
set -e

# Install theme files
mkdir -p "${ROOTFS_DIR}/usr/share/plymouth/themes/mirai/"
cp -r files/mirai/* "${ROOTFS_DIR}/usr/share/plymouth/themes/mirai/"

# Only set theme if Plymouth exists
if [ -x "${ROOTFS_DIR}/usr/bin/plymouth-set-default-theme" ]; then
    chroot "$ROOTFS_DIR" plymouth-set-default-theme -R mirai || true
    echo "[Mirai Plymouth] Plymouth theme applied."
else
    echo "[Mirai Plymouth] Plymouth not installed. Skipping theme setup."
fi

