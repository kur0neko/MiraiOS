#!/bin/bash -e


on_chroot << 'EOF'
apt-get update
apt-get install -y xfce4 lightdm lightdm-gtk-greeter plymouth plymouth-themes xfce4-whiskermenu-plugin
EOF

echo "[Mirai] Starting branding installation..."


PIX_DIR="${ROOTFS_DIR}/usr/share/plymouth/themes/pix"
mkdir -p "$PIX_DIR"

# Replace splash + logo images
install -m 644 files/splash.png "$PIX_DIR/splash.png"
install -m 644 files/splash2.png "$PIX_DIR/logo.png"

on_chroot << 'EOF'
plymouth-set-default-theme -R pix || true
EOF



#change LOGO on rasbian
PIXFLAT_DIR="${ROOTFS_DIR}/usr/share/icons/PiXflat/48x48/places"
mkdir -p "$PIXFLAT_DIR"

install -m 644 files/mirai-logo.png "$PIXFLAT_DIR/rpi-logo.png"

on_chroot << 'EOF'
gtk-update-icon-cache /usr/share/icons/PiXflat || true
EOF

#set defaul wallpaper
WALL_DIR="${ROOTFS_DIR}/usr/share/backgrounds"
mkdir -p "$WALL_DIR"

install -m 644 files/mirai_wallpaper.png \
    "${WALL_DIR}/mirai_wallpaper.png"

#MAKE MIRAI WALLPAPER THE SYSTEM'S DEFAULT

# This helps any DE (XFCE, LXDE, Openbox, Wayfire detect wallpaper as default
mkdir -p "${ROOTFS_DIR}/etc/skel/.config"

#Place a generic wallpaper reference config used by many DEs
echo "/usr/share/backgrounds/mirai_wallpaper.png" > \
    "${ROOTFS_DIR}/etc/skel/.config/mirai_default_wallpaper"

#XFCE auto-loads from skel if XFCE is later installed
mkdir -p "${ROOTFS_DIR}/etc/skel/.config/xfce4/xfconf/xfce-perchannel-xml"
cat > "${ROOTFS_DIR}/etc/skel/.config/xfce4/xfconf/xfce-perchannel-xml/xfce4-desktop.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<channel name="xfce4-desktop" version="1.0">
  <property name="backdrop">
    <property name="screen0">
      <property name="monitor0">
        <property name="image-path" type="string" value="/usr/share/backgrounds/mirai_wallpaper.png"/>
      </property>
    </property>
  </property>
</channel>
EOF

# Wayfire / Weston (if installed later)
mkdir -p "${ROOTFS_DIR}/etc/xdg/wf-shell"
cat << 'EOF' > "${ROOTFS_DIR}/etc/xdg/wf-shell/default.ini"
background = /usr/share/backgrounds/mirai_wallpaper.png
background_mode = fill
EOF

echo "Mirai Branding installation is now completed."

