#!/usr/bin/env python3
"""Pack an actual IDF build's flasher_args.json parts into one offset-0 image.

This is NOT a compiler. IDF builds all input .bin files before this script runs.
No undocumented fixed offsets or hard-coded project file names.
"""
import argparse
import hashlib
import json
from pathlib import Path


def merge(build: Path, output: Path):
    metadata = build / "flasher_args.json"
    obj = json.loads(metadata.read_text(encoding="utf-8"))
    files = obj["flash_files"]
    if not files:
        raise RuntimeError("flash_files is empty — incomplete IDF build")
    segments = []
    end = 0
    for addr, rel in files.items():
        offset = int(addr, 0)
        raw = (build / rel).resolve().read_bytes()
        if not raw:
            raise RuntimeError(f"Empty partition {rel}")
        if offset < 0 or offset + len(raw) > 4 * 1024 * 1024:
            raise RuntimeError(f"Unexpected firmware address/size: {addr} -> {rel}")
        segments.append((offset, raw, rel))
        end = max(end, offset + len(raw))
    if not any(start == 0 for start, _raw, _rel in segments):
        raise RuntimeError("Expected ESP32-S3 bootloader at offset 0x0")
    segments.sort(key=lambda x: x[0])
    last_end = 0
    image = bytearray(b"\xff" * end)
    for start, raw, rel in segments:
        if start < last_end:
            raise RuntimeError(f"Overlapping flash segments at 0x{start:x}")
        image[start:start + len(raw)] = raw
        last_end = start + len(raw)
        print(f"  0x{start:06x}: {rel} ({len(raw):,} bytes)")
    if image[0] != 0xE9:
        raise RuntimeError("Merged bootloader does not have an ESP image header (0xE9)")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    print(f"Merged image: {output}, {len(image):,} bytes, SHA256 {hashlib.sha256(image).hexdigest()}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", default="firmware/esp32/build", type=Path)
    parser.add_argument("--output", default="flasher/assets/firmware-merged.bin", type=Path)
    args = parser.parse_args()
    merge(args.build, args.output)
