#!/bin/bash
set -e

echo "[Mirai Wallpaper] Installing Wayland wallpaper..."

mkdir -p "${ROOTFS_DIR}/usr/share/backgrounds/"

install -m 644 files/usr/share/backgrounds/mirai_wallpaper.png \
    "${ROOTFS_DIR}/usr/share/backgrounds/mirai_wallpaper.png"

mkdir -p "${ROOTFS_DIR}/etc/xdg/wf-shell/"

cat << 'EOF' > "${ROOTFS_DIR}/etc/xdg/wf-shell/default.ini"
background = /usr/share/backgrounds/mirai_wallpaper.png
background_layout = zoom
EOF


