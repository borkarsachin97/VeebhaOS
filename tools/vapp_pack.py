#!/usr/bin/env python3
"""
VeebhaOS VAPP Packaging & Verification CLI Tool
SPDX-License-Identifier: MIT

Usage:
  python3 tools/vapp_pack.py --manifest vapp.json --bin app.bin --output my_app.vapp
  python3 tools/vapp_pack.py --info my_app.vapp
"""

import sys
import os
import struct
import json
import argparse

VAPP_MAGIC = 0x50504156  # "VAPP" in little-endian
VAPP_ABI_VERSION = 1

TYPE_MAP = {
    "game": 0,
    "app": 1,
    "software": 2,
    "extension": 3,
    "utility": 4
}

TYPE_REV_MAP = {v: k for k, v in TYPE_MAP.items()}


def compute_crc16(data: bytes) -> int:
    """Computes CCITT CRC16 matching VeebhaOS vapp_compute_crc16."""
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8) & 0xFFFF
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def pack_vapp(manifest_path: str, bin_path: str, output_path: str, app_id: int = 1):
    with open(manifest_path, 'r', encoding='utf-8') as f:
        manifest = json.load(f)

    name = manifest.get("name", "Untitled App").encode('utf-8')[:31]
    author = manifest.get("author", "Veebha Dev").encode('utf-8')[:23]
    version = manifest.get("version", "1.0.0").encode('utf-8')[:11]
    icon = manifest.get("icon", "PLAY").encode('utf-8')[:7]
    desc = manifest.get("description", "A VeebhaOS Application").encode('utf-8')[:63]
    app_type_str = manifest.get("type", "game").lower()
    app_type = TYPE_MAP.get(app_type_str, 0)
    req_heap = int(manifest.get("heap_budget_kb", 16)) * 1024

    bin_data = b""
    if bin_path and os.path.exists(bin_path):
        with open(bin_path, 'rb') as f:
            bin_data = f.read()
    else:
        # Dummy payload if no binary provided
        bin_data = b"\x90" * 256

    code_size = len(bin_data)
    manifest_app_id = int(manifest.get("app_id", app_id))

    # Format header struct:
    # uint32_t magic (4)
    # uint16_t abi_version (2)
    # uint16_t app_type (2)
    # char name[32] (32)
    # char author[24] (24)
    # char version[12] (12)
    # char icon[8] (8)
    # char desc[64] (64)
    # uint32_t req_heap_bytes (4)
    # uint32_t code_size (4)
    # uint32_t app_id (4)
    # (Total before CRC = 158 bytes)
    # uint16_t crc (2) -> Total = 160 bytes

    header_fmt = "<I H H 32s 24s 12s 8s 64s I I I"
    hdr_data = struct.pack(
        header_fmt,
        VAPP_MAGIC,
        VAPP_ABI_VERSION,
        app_type,
        name.ljust(32, b'\x00'),
        author.ljust(24, b'\x00'),
        version.ljust(12, b'\x00'),
        icon.ljust(8, b'\x00'),
        desc.ljust(64, b'\x00'),
        req_heap,
        code_size,
        manifest_app_id
    )

    crc = compute_crc16(hdr_data)
    full_header = hdr_data + struct.pack("<H", crc)

    with open(output_path, 'wb') as out_f:
        out_f.write(full_header)
        out_f.write(bin_data)

    print(f"[VAPP_PACK] Successfully packed package -> '{output_path}'")
    print(f"            Name: {name.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"            Type: {app_type_str.upper()} (ID: {manifest_app_id})")
    print(f"            Author: {author.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"            Version: {version.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"            Heap Budget: {req_heap // 1024} KB")
    print(f"            Payload Size: {code_size} bytes")
    print(f"            CRC16: 0x{crc:04X}")


def info_vapp(vapp_path: str):
    if not os.path.exists(vapp_path):
        print(f"[ERROR] File not found: {vapp_path}")
        sys.exit(1)

    with open(vapp_path, 'rb') as f:
        data = f.read()

    if len(data) < 162:
        print("[ERROR] File too small to be a valid .vapp package")
        sys.exit(1)

    header_data = data[:160]
    crc_bytes = data[160:162]
    file_crc = struct.unpack("<H", crc_bytes)[0]
    calc_crc = compute_crc16(header_data)

    (magic, abi, app_type, name, author, version, icon, desc, req_heap, code_size, app_id) = struct.unpack(
        "<I H H 32s 24s 12s 8s 64s I I I", header_data
    )

    if magic != VAPP_MAGIC:
        print(f"[ERROR] Invalid magic: 0x{magic:08X} (expected 0x{VAPP_MAGIC:08X})")
        sys.exit(1)

    print(f"=== VeebhaOS VAPP Package Info: {os.path.basename(vapp_path)} ===")
    print(f"  Name:         {name.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"  Author:       {author.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"  Version:      {version.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"  Category:     {TYPE_REV_MAP.get(app_type, 'unknown').upper()}")
    print(f"  App ID:       {app_id}")
    print(f"  Icon Symbol:  {icon.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"  Description:  {desc.decode('utf-8', 'ignore').rstrip(chr(0))}")
    print(f"  Heap Budget:  {req_heap // 1024} KB ({req_heap} B)")
    print(f"  Payload Size: {code_size} B (Total file: {len(data)} B)")
    print(f"  CRC16 Check:  0x{file_crc:04X} ({'VALID' if file_crc == calc_crc else 'MISMATCH'})")


def main():
    parser = argparse.ArgumentParser(description="VeebhaOS VAPP Packaging CLI Tool")
    parser.add_argument("--manifest", help="Path to vapp.json manifest")
    parser.add_argument("--bin", help="Path to executable binary payload")
    parser.add_argument("--output", "-o", help="Output .vapp path")
    parser.add_argument("--app-id", type=int, default=1, help="Unique application ID")
    parser.add_argument("--info", help="Inspect an existing .vapp package")

    args = parser.parse_args()

    if args.info:
        info_vapp(args.info)
    elif args.manifest:
        out = args.output or "app.vapp"
        pack_vapp(args.manifest, args.bin, out, args.app_id)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
