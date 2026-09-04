# BmapImager

跨平台 bmap 镜像烧录器(Windows / macOS / Linux)。用 bmap(block map)格式把磁盘镜像快速、可靠地写入 SD 卡或 USB 存储设备——只写有数据的块、逐块 SHA256 校验,替代 `dd`。

bmap 生态目前只有命令行工具([`yoctoproject/bmaptool`](https://github.com/yoctoproject/bmaptool),Python),本项目补上缺失的图形界面。

## 特性

- 解析 bmap v2.0,按映射区段写入,跳过空洞
- 逐块 SHA256 校验 + bmap 文件自身校验和验证
- 透明解压 `.img` / `.img.gz` / `.img.bz2` / `.img.xz`(libarchive)
- 特权边界倒转:普通进程解析/校验/解压,特权 helper 只做 `open + pwrite + fsync`
- 多层设备安全过滤 + 写入前特权层复验
- 实时进度、可取消

## 架构

```
普通权限进程 (core)                 特权 helper (bmap-writer)
  解析 bmap → 校验文件校验和          复验设备安全
  → 逐块解压 → 逐块 SHA256            open(raw device)
  → {offset, bytes} ──帧协议──▶       pwrite(buf @ off) / fsync
```

## 构建

需要 CMake ≥ 3.21、Qt 6(Core、Widgets、Test)、libarchive。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 项目结构

```
src/
├── core/       # bmapcore 静态库:解析/解压/校验/拷贝(可移植,可测)
├── platform/   # bmapplatform:设备枚举 / 裸写 / 提权(每 OS)
├── writer/     # bmap-writer 特权 helper
└── ui/         # Qt Widgets GUI
tests/          # Qt Test
```

## 状态

- [x] P0 核心引擎(bmap 解析 + 校验 + 解压 + 拷贝)
- [ ] P1 平台层 + writer helper(三 OS)
- [ ] P2 GUI
- [ ] P3 打包发布
