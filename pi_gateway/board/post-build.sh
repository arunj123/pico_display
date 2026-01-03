#!/bin/sh

set -u
set -e

# Buildroot passes the target directory as the first argument
TARGET_DIR=$1

# --- 1. HDMI Console Setup (Original Logic) ---
if [ -e ${TARGET_DIR}/etc/inittab ]; then
    grep -qE '^tty1::' ${TARGET_DIR}/etc/inittab || \
        sed -i '/GENERIC_SERIAL/a\
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # HDMI console' ${TARGET_DIR}/etc/inittab
elif [ -d ${TARGET_DIR}/etc/systemd ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/getty.target.wants"
    ln -sf /lib/systemd/system/getty@.service \
        "${TARGET_DIR}/etc/systemd/system/getty.target.wants/getty@tty1.service"
fi

# --- 2. Fix RPi Zero 2 W WiFi Firmware (64-bit Robust) ---
# placed binary firmware in the pi_gateway/board/overlay/lib/firmware/brcm/brcmfmac43436-sdio.*

# --- 3. Fix RPi Zero 2 W Bluetooth Firmware ---
# Link the standard HCD to the model-specific name
BT_DIR="${TARGET_DIR}/lib/firmware/brcm"

if [ -e ${BT_DIR}/BCM43430B0.hcd ]; then
    ln -sf BCM43430B0.hcd \
        ${BT_DIR}/BCM43430B0.raspberrypi,model-zero-2-w.hcd
fi
