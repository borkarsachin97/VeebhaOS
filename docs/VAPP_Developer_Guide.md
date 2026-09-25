# VeebhaOS VAPP Developer Guide & SDK Manual

## 1. Introduction to `.vapp` Packages

A **`.vapp`** (*Veebha Application Package*) is a self-contained, standalone application binary designed for VeebhaOS feature phone hardware and simulators. It packages application metadata, icon indicators, category classifications, memory bounds, and executable payload into a single file.

---

## 2. Package Architecture & Header Format

Every `.vapp` package begins with a 162-byte binary header followed by the executable bytecode/native payload:

| Field | Size | Type | Description |
| :--- | :--- | :--- | :--- |
| `magic` | 4 bytes | `uint32_t` | Magic Signature: `0x50504156` (`"VAPP"`) |
| `abi_version` | 2 bytes | `uint16_t` | OS ABI Level (Current: `1`) |
| `app_type` | 2 bytes | `uint16_t` | `0`=Game, `1`=App, `2`=Software, `3`=Extension, `4`=Utility |
| `name` | 32 bytes | `char[32]` | Display Name (e.g. `"Tetris Retro"`) |
| `author` | 24 bytes | `char[24]` | Developer / Studio Name |
| `version` | 12 bytes | `char[12]` | SemVer String (e.g. `"1.2.0"`) |
| `icon_symbol` | 8 bytes | `char[8]` | LVGL Icon Symbol or Icon Tag |
| `description` | 64 bytes | `char[64]` | Short description shown in store |
| `req_heap_bytes` | 4 bytes | `uint32_t` | Requested Heap Budget (e.g. `16384` for 16 KB) |
| `code_size` | 4 bytes | `uint32_t` | Executable payload size in bytes |
| `app_id` | 4 bytes | `uint32_t` | Unique Application Identifier |
| `header_crc16` | 2 bytes | `uint16_t` | CCITT CRC16 Checksum |

---

## 3. Project Directory Structure

```
my_app_project/
├── vapp.json          # Application Manifest
├── Makefile           # Build and packaging script
└── src/
    └── main.c         # Application C source code
```

### 3.1 Application Manifest (`vapp.json`)

```json
{
  "name": "Tetris Retro",
  "author": "ArcadeClassics",
  "version": "1.0.0",
  "type": "game",
  "icon": "PLAY",
  "description": "Classic falling tetromino puzzle game",
  "heap_budget_kb": 16,
  "app_id": 6,
  "permissions": ["display", "keypad", "audio"]
}
```

---

## 4. Keypad Mapping Reference

VeebhaOS maps standard 12-key phone keypads as follows:

| Phone Key | PC Simulator Key | Usage in Games & Apps |
| :--- | :--- | :--- |
| **LSK** | `Left Alt` / `F1` | Primary Action / Select / Restart |
| **RSK** | `Right Alt` / `F2` / `Escape` | Secondary Action / Back / Exit |
| **D-Pad Up** / `2` | `Up Arrow` / `Keypad 2` | Move Up / Rotate Piece / Increment |
| **D-Pad Down** / `8`| `Down Arrow` / `Keypad 8`| Move Down / Soft Drop / Decrement |
| **D-Pad Left** / `4`| `Left Arrow` / `Keypad 4`| Move Left / Previous Tab |
| **D-Pad Right** / `6`| `Right Arrow` / `Keypad 6`| Move Right / Next Tab |
| **OK / Enter** / `5` | `Enter` / `Space` / `Keypad 5`| Select / Fire / Rotate |
| `0` / `#` | `Keypad 0` / `#` | Hard Drop / Slam / Toggle Mode |

---

## 5. Packaging & Deployment

Use the `vapp_pack.py` CLI utility to generate packages:

```bash
# Packaging an app:
python3 tools/vapp_pack.py --manifest vapp.json --bin build/app.bin --output /vapps/my_app.vapp

# Inspecting an existing package:
python3 tools/vapp_pack.py --info /vapps/my_app.vapp
```

To install an app on VeebhaOS, simply place the `.vapp` file in the `/vapps/` folder on the SD card (or send it wirelessly via Bluetooth). **VeebhaOS Fun Zone** automatically discovers and indexes up to 64 apps.
