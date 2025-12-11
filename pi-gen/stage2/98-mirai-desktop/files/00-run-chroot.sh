#!/bin/bash
set -e

echo "Ensuring monitor config directories exist..."

mkdir -p /home/pi/.config
mkdir -p /etc/skel/.config

chown -R pi:pi /home/pi/.config


