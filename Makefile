VMLINUZ := vmlinuz
INITRAMFS := initramfs.cpio.gz
INIT_SRC := init.c llm.c
INIT_BIN := /tmp/nymoris-init

CC := x86_64-elf-gcc
CFLAGS := -nostdlib -static -O2 -fno-builtin -fno-tree-vectorize -fno-inline

# Extra QEMU flags (e.g., QEMU_EXTRA="-netdev user,id=net0,hostfwd=tcp::8080-:80")
QEMU_EXTRA ?=

.PHONY: all build run run-gui clean

all: build

$(INIT_BIN): $(INIT_SRC)
	$(CC) $(CFLAGS) -o $@ $(INIT_SRC)

$(INITRAMFS): $(INIT_BIN)
	mkdir -p initramfs/dev initramfs/proc initramfs/sys initramfs/tmp initramfs/bin
	cp $(INIT_BIN) initramfs/init
	chmod +x initramfs/init
	cp -f test_model.nymollm initramfs/model.nymollm 2>/dev/null || true
	cp -f test_model.nymollm.vocab initramfs/model.nymollm.vocab 2>/dev/null || true
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
		-append "console=ttyS0 panic=1" \
		$(QEMU_EXTRA)

run-gui: build
	qemu-system-x86_64 \
		-kernel $(VMLINUZ) \
		-initrd $(INITRAMFS) \
		-m 512M \
		-no-reboot \
		-append "console=tty0 panic=1 quiet loglevel=0" \
		$(QEMU_EXTRA)

clean:
	rm -rf $(INITRAMFS) initramfs
