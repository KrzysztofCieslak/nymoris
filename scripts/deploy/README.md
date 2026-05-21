# Nymoris Real Hardware Deployment

This directory contains scripts and configuration for deploying Nymoris to real x86_64 hardware.

## Bootable ISO

### Prerequisites (Linux)

```bash
# Debian/Ubuntu
sudo apt-get install grub-pc-bin grub-common xorriso

# Fedora/RHEL
sudo dnf install grub2-tools xorriso
```

### Create ISO

```bash
cd nymoris/
make build          # Build initramfs first
bash scripts/deploy/mkiso.sh
```

This creates `nymoris.iso` in the project root.

### Test ISO in QEMU

```bash
qemu-system-x86_64 -cdrom nymoris.iso -m 512M
```

### Write ISO to USB

```bash
# Replace /dev/sdX with your USB device (BE CAREFUL!)
sudo dd if=nymoris.iso of=/dev/sdX bs=4M status=progress
sync
```

The USB stick will boot on both BIOS and UEFI systems.

## Manual Bootloader Setup

If you prefer to set up a bootloader manually (e.g., on a hard drive):

### GRUB2 (BIOS + UEFI)

```bash
# Mount the target partition
sudo mkdir -p /mnt/nymoris
sudo mount /dev/sdX1 /mnt/nymoris

# Install GRUB
sudo grub-install --target=i386-pc /dev/sdX
sudo mkdir -p /mnt/nymoris/boot/grub

# Copy files
sudo cp vmlinuz /mnt/nymoris/boot/
sudo cp initramfs.cpio.gz /mnt/nymoris/boot/
sudo cp scripts/deploy/grub.cfg /mnt/nymoris/boot/grub/grub.cfg

sudo umount /mnt/nymoris
```

### syslinux (BIOS only, smaller)

```bash
sudo syslinux -i /dev/sdX1
sudo dd if=/usr/lib/syslinux/mbr.bin of=/dev/sdX bs=440 count=1
sudo mkdir -p /mnt/nymoris/boot
sudo cp vmlinuz initramfs.cpio.gz /mnt/nymoris/boot/
# Create syslinux.cfg...
```

## Network Boot (PXE)

For network booting, serve the kernel and initramfs via TFTP:

```
/tftpboot/
  pxelinux.0
  vmlinuz
  initramfs.cpio.gz
  pxelinux.cfg/default
```

The `default` file:
```
default nymoris
label nymoris
  kernel vmlinuz
  append initrd=initramfs.cpio.gz console=tty0 panic=1
```
