# Contributing to Rover-Drone Project

Welcome! This project is a family learning project. Contributions from family members are encouraged!

## Quick Start

```bash
# Clone repo
git clone <repo-url>
cd rover-drone

# Build firmware
cd firmware
pio run

# Test ground station
cd ../groundstation
python3 -m py_compile ws_test.py
```

## Commit Convention

Use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types
| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `style` | Formatting, no code change |
| `refactor` | Code restructuring |
| `test` | Adding tests |
| `chore` | Maintenance, build, deps |
| `safety` | Safety-critical changes |

### Examples
```
feat(firmware): add PID auto-tune mode
fix(groundstation): handle WS reconnection
docs: update pinout reference card
safety(firmware): add watchdog timeout
```

## Branch Strategy

```
main          → Protected, only via PR
develop       → Integration branch
feature/*     → Feature branches
bugfix/*      → Bug fixes
safety/*      → Safety-critical fixes
```

## Pull Request Process

1. Create feature branch from `develop`
2. Make changes with clear commits
3. Run tests locally (`pio run`, `python -m py_compile`)
4. Open PR against `develop`
5. Fill PR template completely
6. Self-review before requesting review
7. Address review comments
8. Squash merge to `develop`

## Safety First

**This controls physical hardware.** Always consider:

- Emergency stop must work (button, timeout, WS disconnect)
- Battery critical voltage = immediate motor stop
- Command timeout (500ms) = emergency stop
- Battery < 6.6V = hard stop
- No changes to motor control without review

## Hardware Changes

For hardware modifications:
1. Update `docs/wiring.md` and `docs/pinout_reference.md`
2. Update `firmware/src/pinout.h` if GPIO changes
3. Test on breadboard first
4. Verify with multimeter before powering ESP32

## Code Style

- **C++**: Arduino style, 2-space indent
- **Python**: PEP 8, type hints where useful
- **Comments**: Explain *why*, not *what*
- **Safety comments**: Prefix with `SAFETY:`

## Testing

Before PR:
```bash
# Firmware
cd firmware && pio run

# Ground station
cd groundstation && python3 -m py_compile ws_test.py

# Docs check
ls README.md docs/wiring.md docs/pinout_reference.md
```

## Questions?

Ask in GitHub Discussions or open an issue with `question` label.