# Documentation Skill

This skill ensures KACOS documentation is maintained alongside code changes.

## Documentation Requirement

After EVERY coding task, update or create documentation for the changed component.

## What to Document

### Code Changes
- New modules: add module-level comment explaining purpose
- New public functions: document parameters, return values, and safety invariants
- New commands: update `cmd_help()` in `kernel/src/commands.rs`
- New dependencies: update README.md Prerequisites and Cargo.toml description

### Architecture Changes
- New subsystems (network stack, USB, etc.): update README.md Architecture section
- Memory layout changes: update both README.md and CLAUDE.md
- Boot sequence changes: update README.md Boot Sequence

### Security-Relevant Changes
- New input parsers: document validation strategy and threat model
- New `unsafe` blocks: document safety invariants in code comments
- New drivers: document hardware assumptions and QEMU-specific behavior

## Documentation Files to Maintain

| File | When to Update |
|------|---------------|
| README.md | New features, changed architecture, updated roadmap |
| CLAUDE.md | Build process changes, architecture changes, new pitfalls |
| SECURITY.md | New threat surfaces, changed security boundaries |

## Documentation Checklist (apply before finishing)

- [ ] New public APIs have doc comments
- [ ] New commands appear in `help` output
- [ ] README.md reflects current capabilities and status
- [ ] CLAUDE.md reflects current build process and architecture
- [ ] SECURITY.md updated if threat model changed
- [ ] All documentation is committed and pushed to GitHub
