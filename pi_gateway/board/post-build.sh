#!/bin/sh

set -u
set -e

# Buildroot passes the target directory as the first argument
TARGET_DIR=$1

# --- 1. HDMI Console Setup ---
if [ -e ${TARGET_DIR}/etc/inittab ]; then
    grep -qE '^tty1::' ${TARGET_DIR}/etc/inittab || \
        sed -i '/GENERIC_SERIAL/a\
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # HDMI console' ${TARGET_DIR}/etc/inittab
elif [ -d ${TARGET_DIR}/etc/systemd ]; then
    mkdir -p "${TARGET_DIR}/etc/systemd/system/getty.target.wants"
    ln -sf /lib/systemd/system/getty@.service \
        "${TARGET_DIR}/etc/systemd/system/getty.target.wants/getty@tty1.service"
fi

# --- 2. RPi Zero 2 W WiFi Firmware ---
# The WiFi firmware files for the RPi Zero 2 W need to be linked from
# the existing RPi 3 firmware files.

FIRMWARE_DIR="${TARGET_DIR}/lib/firmware/brcm"
ROOT_FIRMWARE_DIR="${TARGET_DIR}/lib/firmware"

mkdir -p "$FIRMWARE_DIR"

# --- PART 1: Wifi Firmware ---
echo "  [WiFi] Downloading firmware files..."
wget -O "$FIRMWARE_DIR/brcmfmac43436-sdio.bin" "https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/master/brcm/brcmfmac43436-sdio.bin"
wget -O "$FIRMWARE_DIR/brcmfmac43436-sdio.txt" "https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/master/brcm/brcmfmac43436-sdio.txt"
wget -O "$FIRMWARE_DIR/brcmfmac43436-sdio.clm_blob" "https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/master/brcm/brcmfmac43436-sdio.clm_blob"
wget -O "$FIRMWARE_DIR/LICENCE.broadcom_bcm43xx" "https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/master/LICENCE.broadcom_bcm43xx"

# Fix symlinks for Zero 2 W (The "43430b0" fix)
# The Zero 2 W identifies as 43430b0 but uses 43436 firmware files.
ln -sf brcmfmac43436-sdio.bin "$FIRMWARE_DIR/brcmfmac43430b0-sdio.bin"
ln -sf brcmfmac43436-sdio.txt "$FIRMWARE_DIR/brcmfmac43430b0-sdio.txt"
ln -sf brcmfmac43436-sdio.clm_blob "$FIRMWARE_DIR/brcmfmac43430b0-sdio.clm_blob"
# Model specific links
ln -sf brcmfmac43436-sdio.bin "$FIRMWARE_DIR/brcmfmac43430b0-sdio.raspberrypi,model-zero-2-w.bin"
ln -sf brcmfmac43436-sdio.txt "$FIRMWARE_DIR/brcmfmac43430b0-sdio.raspberrypi,model-zero-2-w.txt"

# --- PART 2: Regulatory Database ---
echo "  [WiFi] Downloading Regulatory DB..."
wget -O "$ROOT_FIRMWARE_DIR/regulatory.db" "https://git.kernel.org/pub/scm/linux/kernel/git/sforshee/wireless-regdb.git/plain/regulatory.db?id=HEAD"
wget -O "$ROOT_FIRMWARE_DIR/regulatory.db.p7s" "https://git.kernel.org/pub/scm/linux/kernel/git/sforshee/wireless-regdb.git/plain/regulatory.db.p7s?id=HEAD"

# --- 3. Fix RPi Zero 2 W Bluetooth Firmware ---
echo "  [Bluetooth] Downloading BCM43430B0.hcd..."
BT_FIRMWARE_URL="https://raw.githubusercontent.com/RPi-Distro/bluez-firmware/master/broadcom/BCM43430B0.hcd"
BT_DEST_DIR="${TARGET_DIR}/lib/firmware/brcm"

mkdir -p "$BT_DEST_DIR"

# Download the missing Bluetooth firmware
wget -O "$BT_DEST_DIR/BCM43430B0.hcd" "$BT_FIRMWARE_URL"

# Link the standard HCD to the model-specific name
# This fixes the "Patch file not found" error
ln -sf BCM43430B0.hcd "${BT_DEST_DIR}/BCM43430B0.raspberrypi,model-zero-2-w.hcd"

echo "Post-build script finished successfully."