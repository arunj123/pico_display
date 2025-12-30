#!/bin/bash
set -e

# Configuration
# We place buildroot one level up or in a 'deps' folder to keep repo clean
BUILDROOT_DIR="../buildroot"
# output directory for the build artifacts
OUTPUT_DIR="../build_pi_output"
# Path to your custom directory (relative to this script)
EXTERNAL_TREE=$(readlink -f "./pi_gateway")

# 1. Download Buildroot if missing
if [ ! -d "$BUILDROOT_DIR" ]; then
    echo "Buildroot not found. Cloning..."
    git clone https://git.buildroot.net/buildroot $BUILDROOT_DIR
    # Checkout a stable branch for consistency (Optional but recommended)
    cd $BUILDROOT_DIR && git checkout 2024.02.x && cd -
fi

# 2. Configure (only if .config doesn't exist)
if [ ! -f "$OUTPUT_DIR/.config" ]; then
    echo "Configuring Buildroot..."
    make -C $BUILDROOT_DIR \
         BR2_EXTERNAL="$EXTERNAL_TREE" \
         O="$OUTPUT_DIR" \
         pizero_defconfig
else
    echo "Buildroot already configured. Skipping defconfig..."
fi

# 3. Build
echo "Building Image..."
make -C $OUTPUT_DIR

echo "Done! Image is at: $OUTPUT_DIR/images/sdcard.img"