RUST_TARGET := kernel/x86_64-nymoris.json
LINKER_SCRIPT := linker.ld
KERNEL := kernel/target/x86_64-nymoris/release/nymoris
ISO := nymoris.iso
BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

RUSTFLAGS := -C link-arg=-T../$(LINKER_SCRIPT)
CARGO := $(HOME)/.cargo/bin/cargo +nightly
CARGO_BUILD_FLAGS := -Zbuild-std=core,alloc,compiler_builtins -Zbuild-std-features=compiler-builtins-mem -Zjson-target-spec --target x86_64-nymoris.json --release

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

# GUI mode with serial output to terminal (for debugging).
# Type commands in the terminal window, output appears in both terminal and QEMU window.
# Do NOT close the terminal while QEMU is running.
run-gui: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial stdio -no-shutdown -no-reboot -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# Pure GUI mode - no terminal serial attachment. QEMU window is the only interface.
# Serial output is logged to qemu-serial.log. Closing the terminal does NOT kill QEMU.
# Type commands inside the QEMU window (USB keyboard).
run-gui-pure: iso
	@echo "Starting QEMU in pure GUI mode..."
	@echo "Serial output logged to qemu-serial.log"
	@qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# GUI with PS/2 keyboard only (no USB). Use this if USB keyboard doesn't work.
run-gui-ps2: iso
	@echo "Starting QEMU with PS/2 keyboard only..."
	@echo "Serial output logged to qemu-serial.log"
	@qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -cpu qemu64,+sse,+sse2 -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# Debug GUI mode - logs interrupts to qemu-serial.log. Use if QEMU quits on keypress.
run-gui-debug: iso
	@echo "Starting QEMU with interrupt debug logging..."
	@echo "Serial + debug output logged to qemu-serial.log"
	@qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -d int -cpu qemu64,+sse,+sse2 -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

run-debug: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -nographic -no-reboot -d int -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# Alternative machine type (Q35 chipset instead of i440fx/PIIX3).
# Uses ICH9 chipset which has different PS/2 and USB controller emulation.
run-gui-q35: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -cpu qemu64,+sse,+sse2 -machine q35 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# VNC mode - no Cocoa window, connect via VNC viewer (localhost:5900).
# This bypasses the Cocoa backend entirely and may avoid keyboard-related crashes.
run-vnc: iso
	@echo "Starting QEMU with VNC on port 5900..."
	@echo "Connect with: open vnc://localhost:5900"
	@echo "Serial output logged to qemu-serial.log"
	@qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -vnc :0 -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80

# Maximum debug mode - logs interrupts, CPU resets, guest errors.
# Use this if QEMU quits unexpectedly to capture diagnostic info.
run-gui-maxdebug: iso
	@echo "Starting QEMU with maximum debug logging..."
	@echo "Output logged to qemu-debug.log"
	@qemu-system-x86_64 -cdrom $(ISO) -m 256M -serial file:qemu-serial.log -no-shutdown -no-reboot -d int,cpu_reset,guest_errors,unimp -cpu qemu64,+sse,+sse2 -usb -device usb-kbd -device virtio-net-pci,netdev=net0 -netdev user,id=net0,hostfwd=tcp::8080-:80 2> qemu-debug.log

clean:
	rm -rf $(BUILD_DIR) $(ISO)
	cd kernel && $(CARGO) clean
