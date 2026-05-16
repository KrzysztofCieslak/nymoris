RUST_TARGET := kernel/x86_64-nymoris.json
LINKER_SCRIPT := linker.ld
KERNEL := kernel/target/x86_64-nymoris/release/nymoris
ISO := nymoris.iso
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

RUSTFLAGS := -C link-arg=-T../$(LINKER_SCRIPT)
CARGO := $(HOME)/.cargo/bin/cargo +nightly
CARGO_BUILD_FLAGS := -Zbuild-std=core,compiler_builtins -Zbuild-std-features=compiler-builtins-mem -Zjson-target-spec --target x86_64-nymoris.json --release

.PHONY: all iso run clean

all: iso

$(KERNEL): $(shell find kernel/src -name '*.rs') kernel/Cargo.toml $(RUST_TARGET) $(LINKER_SCRIPT)
	cd kernel && RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build $(CARGO_BUILD_FLAGS)

iso: $(ISO)

$(ISO): $(KERNEL) limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin limine/BOOTX64.EFI
	@echo "Building ISO..."
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/limine
	cp $(KERNEL) $(ISO_DIR)/boot/nymoris
	cp limine.conf $(ISO_DIR)/boot/limine/
	cp limine/limine-bios.sys $(ISO_DIR)/boot/limine/
	cp limine/limine-bios-cd.bin $(ISO_DIR)/boot/limine/
	cp limine/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/
	mkdir -p $(ISO_DIR)/EFI/BOOT
	cp limine/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_DIR) -o $(ISO)
	./limine/limine bios-install $(ISO)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -nographic -no-reboot -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

run-gui: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial stdio -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

run-debug: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -nographic -no-reboot -d int -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

clean:
	rm -rf $(BUILD_DIR) $(ISO)
	cd kernel && $(CARGO) clean
