# Testing Skill

This skill ensures all KACOS kernel changes are verified before completion.

## Build Verification

After EVERY code change, run:

```bash
make iso
```

This builds the kernel and creates a bootable ISO. The build must succeed with zero warnings and zero errors.

## Runtime Verification

After the build succeeds, test in QEMU:

```bash
make run-gui
```

Verify the golden path:
1. Kernel boots without panic
2. "[OK] Network initialized" appears
3. Shell prompt is responsive
4. Type `help` — command list displays correctly
5. Type `ping 10.0.2.2` — ping succeeds
6. Type `about` — system info displays

## Regression Testing

After functional changes, verify existing features still work:
- Keyboard input: type characters, use backspace, press Enter
- USB keyboard: keys do not repeat (de-duplication works)
- Framebuffer: text renders correctly, clear screen works
- Serial output: COM1 receives boot messages
- Network: ARP resolution, ICMP echo reply

## Test Checklist (apply before finishing)

- [ ] `make iso` builds successfully with no errors
- [ ] `make run-gui` boots to shell
- [ ] Keyboard input works (type, backspace, enter)
- [ ] `ping 10.0.2.2` returns reply
- [ ] No new warnings introduced
- [ ] If a bug was fixed, the original issue is no longer reproducible
