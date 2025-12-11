#!/bin/bash
set -e

echo "[Updating OS metadata..."

cat <<EOF > /etc/os-release
PRETTY_NAME="Mirai OS 1.3 (lastlight)"
NAME="MiraiOS"
ID=mirai
VERSION_ID="1.3"
VERSION="1.3 lastlight"
HOME_URL="https://mirai-os.local"
SUPPORT_URL="https://mirai-os.local/support"
BUG_REPORT_URL="https://mirai-os.local/issues"
EOF

cat <<EOF > /etc/issue
Mirai OS 1.3 lastlight \\n \\l
EOF

cat <<EOF > /etc/lsb-release
DISTRIB_ID=MiraiOS
DISTRIB_RELEASE=1.3
DISTRIB_CODENAME=lastlight
DISTRIB_DESCRIPTION="Mirai OS 1.3 lastlight"
EOF

cat << 'EOF' > /etc/motd
███╗   ███╗██╗██████╗  █████╗ ██╗
████╗ ████║██║██╔══██╗██╔══██╗██║
██╔████╔██║██║██████╔╝███████║██║
██║╚██╔╝██║██║██╔══██╗██╔══██║██║
██║ ╚═╝ ██║██║██║  ██║██║  ██║██║
╚═╝     ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝

Mirai OS 1.3 — lastlight edition

Welcome to Mirai OS
EOF

echo "[Mirai Branding] Completed!"

