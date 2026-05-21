#!/bin/bash
# Create a bootable ISO image for Nymoris
# Run this on a Linux machine (or in a Docker container)
#
# Prerequisites:
#   grub-mkrescue (Debian/Ubuntu: apt-get install grub-pc-bin grub-common xorriso)
#
# Usage:
#   ./scripts/deploy/mkiso.sh
#   # Writes nymoris.iso to the project root

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_iso"
ISO_PATH="${PROJECT_ROOT}/nymoris.iso"

echo "[*] Building Nymoris bootable ISO..."

# Clean up previous build
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}/boot/grub"

# Copy kernel and initramfs
cp "${PROJECT_ROOT}/vmlinuz" "${BUILD_DIR}/boot/"
cp "${PROJECT_ROOT}/initramfs.cpio.gz" "${BUILD_DIR}/boot/"

# Copy GRUB config
cp "${SCRIPT_DIR}/grub.cfg" "${BUILD_DIR}/boot/grub/grub.cfg"

# Create ISO with GRUB
echo "[*] Running grub-mkrescue..."
grub-mkrescue \
    -o "${ISO_PATH}" \
    "${BUILD_DIR}" \
    --modules="part_msdos part_gpt fat iso9660 linux multiboot2 normal configfile boot" \
    2>/dev/null

# Clean up
rm -rf "${BUILD_DIR}"

echo "[*] Done: ${ISO_PATH}"
echo ""
echo "To test in QEMU:"
echo "  qemu-system-x86_64 -cdrom ${ISO_PATH} -m 512M"
echo ""
echo "To write to USB (replace /dev/sdX with your device):"
echo "  sudo dd if=${ISO_PATH} of=/dev/sdX bs=4M status=progress"
