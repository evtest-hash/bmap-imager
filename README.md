# BmapImager

A cross-platform bmap image flasher (Windows / macOS / Linux). Writes disk images to SD cards and USB storage quickly and reliably using the bmap (block map) format: only mapped blocks are written and each is SHA-256 verified — a safer, faster alternative to `dd`.

The bmap ecosystem currently only has a command-line tool ([`yoctoproject/bmaptool`](https://github.com/yoctoproject/bmaptool), Python). This project adds the missing GUI.

## Features

- Parses bmap v2.0 and writes only the mapped ranges, skipping holes
- Per-range SHA-256 verification + bmap file checksum validation
- Transparent decompression of `.img` / `.img.gz` / `.img.bz2` / `.img.xz` (libarchive)
- Inverted privilege boundary: the unprivileged process parses/verifies/decompresses; the privileged helper only does `open + pwrite + fsync`
- Multi-layer device safety filtering + re-validation in the privileged layer
- Live progress and cancellation

## Architecture

```
unprivileged process (core)              privileged helper (bmap-writer)
  parse bmap → verify file checksum        re-validate device
  → decompress → per-range SHA-256         open(raw device)
  → {offset, bytes} ──frame protocol──▶    pwrite(buf @ off) / fsync
```

## Building

Requires CMake ≥ 3.21, Qt 6 (Core, Widgets, Test), and libarchive.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Layout

```
src/
├── core/       # bmapcore static library: parse / verify / decompress / copy
├── platform/   # bmapplatform: device enumeration / raw write / elevation (per-OS)
├── writer/     # bmap-writer privileged helper
└── ui/         # Qt Widgets GUI
tests/          # Qt Test
```

## Status

- [x] P0 core engine (bmap parse + verify + decompress + copy)
- [ ] P1 platform layer + writer helper (3 OS)
- [ ] P2 GUI
- [ ] P3 packaging
