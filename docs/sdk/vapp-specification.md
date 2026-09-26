# VAPP Package Specification

This document details the binary specification, header format, metadata schemas, and validation requirements for **`.vapp`** (*Veebha Application Package*) files.

---

## 1. Specification Overview

A `.vapp` file is a standalone, single-file binary container designed for dynamic discovery and launching on VeebhaOS.

```text
+-------------------------------------------------------+
|  VAPP Header (162 bytes, struct vapp_header_t)        |
|  - Magic ('VAPP' / 0x50504156)                        |
|  - ABI Version, Type, Heap Budget, App ID             |
|  - Metadata Strings (Name, Author, Version, Desc)     |
|  - Header Checksum (CCITT CRC16)                      |
+-------------------------------------------------------+
|  Executable Code Payload (Native MIPS32 / Bytecode)   |
|  - Length defined by header.code_size                 |
+-------------------------------------------------------+
|  Optional Embedded Assets (Icons, sound tables, etc.) |
+-------------------------------------------------------+
```

---

## 2. Header Structure

The binary header is defined in [`sdk/include/veebha_vapp.h`](file:///home/vixxkigoli/pm/VeebhaOS/sdk/include/veebha_vapp.h):

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* 0x50504156 ("VAPP") */
    uint16_t abi_version;     /* Current version: 1 */
    uint16_t app_type;        /* 0=Game, 1=App, 2=Software, 3=Extension, 4=Utility */
    char     name[32];        /* Null-terminated human-readable app name */
    char     author[24];      /* Null-terminated author/studio name */
    char     version[12];     /* SemVer string e.g. "1.2.0" */
    char     icon_symbol[8];  /* LVGL icon symbol or glyph tag */
    char     description[64]; /* Description displayed in Fun Zone store */
    uint32_t req_heap_bytes;  /* Maximum dynamic memory budget (e.g. 16384) */
    uint32_t code_size;       /* Size of binary code payload following header */
    uint32_t app_id;          /* Unique application identifier */
    uint16_t header_crc16;    /* CCITT CRC16 checksum of bytes 0..159 */
} vapp_header_t;
```

### Field Reference Table

| Field Offset | Size | Type | Field Name | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0x00` | 4 bytes | `uint32_t` | `magic` | Must be `0x50504156` (`"VAPP"`) |
| `0x04` | 2 bytes | `uint16_t` | `abi_version` | Target OS ABI level (`1`) |
| `0x06` | 2 bytes | `uint16_t` | `app_type` | Category: `0`=Game, `1`=App, `2`=Software, `3`=Extension, `4`=Utility |
| `0x08` | 32 bytes| `char[32]` | `name` | Human-readable app name (e.g. `"Space Shooter"`) |
| `0x28` | 24 bytes| `char[24]` | `author` | Studio or developer name |
| `0x40` | 12 bytes| `char[12]` | `version` | SemVer formatted version string |
| `0x4C` | 8 bytes | `char[8]`  | `icon_symbol` | Symbol glyph (e.g. `LV_SYMBOL_PLAY`) |
| `0x54` | 64 bytes| `char[64]` | `description`| Description shown in Fun Zone store |
| `0x94` | 4 bytes | `uint32_t` | `req_heap_bytes` | Heap allocation fence (e.g. 16 KB) |
| `0x98` | 4 bytes | `uint32_t` | `code_size` | Size of executable code payload in bytes |
| `0x9C` | 4 bytes | `uint32_t` | `app_id` | Unique numeric application identifier |
| `0xA0` | 2 bytes | `uint16_t` | `header_crc16` | CCITT CRC16 of bytes `0x00` to `0x9F` |

---

## 3. Checksum Algorithm (CCITT CRC16)

The checksum protects the package against corrupt transfers:

```c
uint16_t vapp_compute_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
```

---

## 4. Application Manifest (`vapp.json`)

Developers describe their application using a JSON manifest:

```json
{
  "name": "Space Shooter",
  "author": "RetroGames",
  "version": "1.0.0",
  "type": "game",
  "icon": "PLAY",
  "description": "Arcade space combat & alien invasion",
  "heap_budget_kb": 16,
  "app_id": 7,
  "permissions": ["display", "keypad", "audio"]
}
```

---

## 5. Packaging CLI (`vapp_pack.py`)

Packages are assembled using the Python CLI utility in [`tools/vapp_pack.py`](file:///home/vixxkigoli/pm/VeebhaOS/tools/vapp_pack.py):

```bash
# Packaging a binary into a .vapp package:
python3 tools/vapp_pack.py --manifest vapp.json --bin build/app.bin --output vapps/space_shooter.vapp

# Inspecting header metadata and verifying checksum:
python3 tools/vapp_pack.py --info vapps/space_shooter.vapp
```
