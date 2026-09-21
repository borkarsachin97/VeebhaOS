# VeebhaOS

**VeebhaOS** is a modular, zero-coordinate embedded operating system engineered for feature phones.

While the primary target silicon is the **RDA8809** (104 MHz MIPS XCPU, 8 MB RAM, 4 MB Flash, ILI9225G LCD), VeebhaOS follows a **Simulator-First Verification Roadmap**: the entire operating system, window manager, typography pipeline, and declarative template engine are built and verified on a Linux desktop SDL2 simulator before touching physical silicon.

---

## Key Architectural Principles

1. **Simulator-First Development:** Native GCC/Clang desktop simulator with integer-scaled SDL2 rendering.
2. **Graphics Subsystem:** LVGL v9 (unmodified stock upstream) with a strict **2.0 MB** dynamic memory pool allocation fence (`LV_MEM_SIZE`) to mirror target hardware RAM limits.
3. **Display Geometry (176 × 220 Portrait, RGB565):**
   - Fixed **18px** Status Bar at the top (Clock, Battery, Signal, Status icons).
   - Elastic **184px** Content Viewport in the middle (Zero-coordinate declarative templates).
   - Fixed **20px** Softkey Bar at the bottom (Left Softkey / Right Softkey).
4. **Zero-Coordinate UI Rule:** Applications never hardcode pixel coordinates. All UI is dynamically generated using 5 declarative template contracts (List, Grid, Dialog, Editor, Media) backed by Flexbox/Grid layouts.
5. **Licensing Firewall & Clean Room:** Core OS, SDK, and apps are licensed under the **MIT License** (`SPDX-License-Identifier: MIT`).

---

## Quick Start (Desktop Simulator)

### Prerequisites (Debian / Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev
```

### Build & Run

```bash
# Using Makefile helper:
make sim

# Or using CMake directly:
cmake -B build
cmake --build build
./build/veebhaos_sim
```

### PC Keyboard Mapping

| Feature Phone Key | PC Keyboard Key |
|---|---|
| **D-Pad Up / Down / Left / Right** | Arrow Keys |
| **D-Pad Center / OK** | Return / Space |
| **Left Softkey (LSK)** | Left Alt / F1 |
| **Right Softkey (RSK)** | Right Alt / F2 / Escape |
| **Keypad 0–9, \*, #** | Numpad / Alphanumeric `0`–`9`, `*`, `#` |
| **Call Key** | `C` (Short: Dial / Long: Quick Settings) |
| **End / Power Key** | `E` |

---

## License

VeebhaOS is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.
