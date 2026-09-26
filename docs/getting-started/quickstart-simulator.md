# Desktop Simulator Quickstart Guide

This guide covers building, running, and debugging VeebhaOS on desktop environments using the SDL2 simulator.

---

## 1. Prerequisites

The VeebhaOS desktop simulator requires a C99 compiler, CMake, and the SDL2 development libraries.

### Debian / Ubuntu / Linux Mint
```bash
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev
```

### Fedora / RHEL
```bash
sudo dnf install -y gcc cmake make SDL2-devel
```

### Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake sdl2
```

### macOS (Homebrew)
```bash
brew install cmake sdl2
```

---

## 2. Building the Simulator

### Fast Build via Makefile
The root `Makefile` provides convenient helper commands:
```bash
# Build binary (build/veebha_os)
make build

# Build and launch simulator immediately
make sim
```

### Manual Build via CMake
If you prefer building directly with CMake:
```bash
cmake -B build -S . -DBOARD=simulator
cmake --build build -j$(nproc)
./build/veebha_os
```

---

## 3. Simulator Features & Keypad Controls

The simulator opens a 176×220 portrait window (scaled smoothly on high-DPI displays) and routes PC keyboard input to the hardware keypad driver.

### Keypad Mapping Table

| Feature Phone Key | PC Key | Description |
| :--- | :--- | :--- |
| **D-Pad Up** | `Up Arrow` / `Keypad 8` | Move focus upward / game move up |
| **D-Pad Down** | `Down Arrow` / `Keypad 2` | Move focus downward / game move down |
| **D-Pad Left** | `Left Arrow` / `Keypad 4` | Move left / previous item / game move left |
| **D-Pad Right** | `Right Arrow` / `Keypad 6` | Move right / next item / game move right |
| **OK / Enter** | `Enter` / `Space` / `Keypad 5` | Confirm selection / fire weapon / rotate |
| **Left Softkey (LSK)** | `Left Alt` / `F1` | Trigger action displayed above left softkey |
| **Right Softkey (RSK)**| `Right Alt` / `F2` / `Escape`| Trigger action displayed above right softkey (Back/Exit)|
| **Numeric 0–9** | Top row `0`–`9` / Numpad | Digit input, T9 text entry, speed dial |
| **Star Key (\*)** | `*` / `Keypad *` | Hold (600ms): Multitasking Task Switcher |
| **Pound Key (#)** | `#` / `Shift+3` | Toggle T9 text entry mode |
| **Call Key (Green)** | `C` | Short: Open dialer / Hold (600ms): Quick Settings Drawer |
| **End Key (Red)** | `E` | Minimize active app / Return to Home screen |

### Mock Hardware IRQs (Simulator Only)
* **F5**: Simulate incoming cellular phone call.
* **F6**: Simulate incoming SMS text message.
* **F7**: Simulate MicroSD card hotplug insertion/removal.

---

## 4. Running Automated Tests

VeebhaOS includes an automated verification runner built directly into the binary:

```bash
# Run headless automated test suite
make test
```
The test suite executes 60 phases across 1096 frames verifying typography, i18n, NVRAM persistence, window navigation, and 60 FPS arcade games.

### Memory Verification with AddressSanitizer
To verify that no memory leaks, use-after-free, or buffer overflows exist:
```bash
make asan
```
This compiles with `-fsanitize=address,undefined` and executes the full verification suite.
