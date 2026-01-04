#!/bin/bash
set -e

BOARD_DIR="$(dirname $0)"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

rm -rf "${GENIMAGE_TMP}"

# Create the seedrng directory in the output folder
mkdir -p "${BINARIES_DIR}/seedrng"

# Optional: Create a placeholder file to ensure the directory is preserved
# (seedrng will overwrite/ignore this later)
touch "${BINARIES_DIR}/seedrng/seed.no-credit"

genimage                           \
	--rootpath "${TARGET_DIR}"     \
	--tmppath "${GENIMAGE_TMP}"    \
	--inputpath "${BINARIES_DIR}"  \
	--outputpath "${BINARIES_DIR}" \
	--config "${GENIMAGE_CFG}"

exit $?