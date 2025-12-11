#!/bin/bash
set -e

# Correct LightDM artwork directory
mkdir -p "${ROOTFS_DIR}/usr/share/raspberrypi-artwork/"

# Install Mirai login image
install -m 644 files/usr/share/images/mirai_login.png \
    "${ROOTFS_DIR}/usr/share/raspberrypi-artwork/mirai_login.png"

# Correct LightDM config path for Bookworm
GREETER_CFG="${ROOTFS_DIR}/etc/lightdm/lightdm-gtk-greeter.conf.d/01_raspberrypi.conf"

# Ensure config directory exists
mkdir -p "${ROOTFS_DIR}/etc/lightdm/lightdm-gtk-greeter.conf.d/"

# Write background line (override Raspberry Pi default)
echo "background=/usr/share/raspberrypi-artwork/mirai_login.png" > "$GREETER_CFG"

