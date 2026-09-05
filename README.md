# BmapImager

A cross-platform bmap image flasher (Windows / macOS / Linux). Writes disk images to SD cards and USB storage quickly and reliably using the bmap (block map) format: only mapped blocks are written and each is SHA-256 verified — a safer, faster alternative to `dd`.

The bmap ecosystem currently only has a command-line tool ([`yoctoproject/bmaptool`](https://github.com/yoctoproject/bmaptool), Python). This project adds the missing GUI.

## Features

- Parses bmap v2.0 and writes only the mapped ranges, skipping holes
- Per-range SHA-256 verification + bmap file checksum validation
- Transparent decompression via libarchive: `.img`, `.img.gz`, `.img.bz2`, `.img.xz`, `.img.lzma`, `.img.zst`
- Inverted privilege boundary: the unprivileged process parses/verifies/decompresses; the privileged helper only does `open + pwrite + fsync`
- Multi-layer device safety filtering + re-validation in the privileged layer before writing
- Live progress and cancellation
- Drag-and-drop image/bmap files
- Device detail (capacity / bus type) and same-name img/bmap auto-pairing

## Architecture

```
unprivileged process (core)              privileged helper (bmap-writer)
  parse bmap → verify file checksum        re-validate device
  → decompress → per-range SHA-256         open(raw device)
  → {offset, bytes} ──frame protocol──▶    pwrite(buf @ off) / fsync
```

The privileged helper (`bmap-writer`) is a small CLI that only opens the raw device and writes verified bytes. It never parses the bmap or decompresses the image, so a malformed bmap can only crash the unprivileged process, never the root helper.

## Building

Requires CMake ≥ 3.21, Qt 6 (Core, Widgets, Test), and libarchive.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

On Windows, libarchive is installed automatically via vcpkg manifest mode (see `vcpkg.json`) with default features disabled to keep the build fast.

## Layout

```
src/
├── core/       # bmapcore static library: parse / verify / decompress / copy
├── platform/   # bmapplatform: device enumeration / raw write / elevation (per-OS)
├── writer/     # bmap-writer privileged helper
└── ui/         # Qt Widgets GUI
tests/          # Qt Test
packaging/      # macOS .app Info.plist
```

## Status

- [x] P0 core engine (bmap parse + verify + decompress + copy)
- [x] P1 platform layer + writer helper (Windows / macOS / Linux)
- [x] P2 Qt Widgets GUI
- [ ] P3 packaging & release — macOS `.app` is assembled and passes CI; not yet released
