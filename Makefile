VMLINUZ := vmlinuz
INITRAMFS := initramfs.cpio.gz
INIT_SRC := init.c
INIT_BIN := /tmp/nymoris-init

CC := x86_64-elf-gcc
CFLAGS := -nostdlib -static -O2 -mno-sse -mno-sse2 -fno-builtin

.PHONY: all build run run-gui clean

all: build

$(INIT_BIN): $(INIT_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(INITRAMFS): $(INIT_BIN)
	mkdir -p initramfs/dev initramfs/proc initramfs/sys initramfs/tmp initramfs/bin
	cp $(INIT_BIN) initramfs/init
	chmod +x initramfs/init
	cd initramfs && find . | cpio -o -H newc 2>/dev/null | gzip > ../$(INITRAMFS)
	rm -rf initramfs

build: $(INITRAMFS)

run: build
	qemu-system-x86_64 \
		-kernel $(VMLINUZ) \
		-initrd $(INITRAMFS) \
		-m 512M \
		-nographic \
		-no-reboot \
		-append "console=ttyS0 panic=1"

run-gui: build
	qemu-system-x86_64 \
		-kernel $(VMLINUZ) \
		-initrd $(INITRAMFS) \
		-m 512M \
		-no-reboot \
		-append "console=tty0"

clean:
	rm -rf $(INITRAMFS) initramfs
