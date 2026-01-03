#!/bin/bash
set -e

# --- FIX FOR WSL2: Remove Windows paths with spaces ---
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
# ------------------------------------------------------

# Config
REPO_ROOT=$(pwd)
BUILDROOT_DIR="${REPO_ROOT}/buildroot"
EXTERNAL_TREE="${REPO_ROOT}/pi_gateway"
OUTPUT_DIR="${REPO_ROOT}/output"
BR_BRANCH="2025.11.x"  # The branch you requested

# 1. Clone Buildroot if missing
if [ ! -d "$BUILDROOT_DIR" ]; then
    echo "Cloning Buildroot (${BR_BRANCH})..."
    git clone -b $BR_BRANCH https://git.buildroot.net/buildroot $BUILDROOT_DIR
fi

# 2. Configure (if no .config exists)
if [ ! -f "$OUTPUT_DIR/.config" ]; then
    echo "Configuring..."
    make -C $BUILDROOT_DIR BR2_EXTERNAL="$EXTERNAL_TREE" O="$OUTPUT_DIR" raspberrypizero2w_64_defconfig
fi

# 3. Build
echo "Building..."
make -C $OUTPUT_DIR

echo "Done! Image: $OUTPUT_DIR/images/sdcard.img"