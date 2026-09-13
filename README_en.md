# sprig

<p align="center">
  <img src="images/execution.png" alt="Execution flow" width="600">
</p>

English version: README_en.md · 中文版：[README_zh-CN.md](README_zh-CN.md) · Original author's version: [README.md](README.md)

> **Credits** — original sprig: [R0rt1z2](https://github.com/R0rt1z2).
> Android boot-restore (chainload) rework and the first bootable fork: 秋逸(逸) &lt;2898684403@qq.com&gt;.
> MT6989 (Xiaomi rothko) v5 port and current maintenance: [woaphone](https://github.com/woaphone).
> Many thanks to the first two — this repository stands on their work.

## What

Yet another example built on the underlying vulnerability used in [fenrir](https://github.com/R0rt1z2/fenrir): a tiny payload replaces the `bl2_ext` image in the LK partition of modern ARMv8 MediaTek devices, runs in EL3, and patches the Preloader in memory to disable SBC / SLA / DAA checks. This allows booting unsigned DAs with `penumbra` or `mtkclient` for unrestricted flash/dump operations.

You will see two Preloader ports: the first disappears within ~2 seconds, the second (exposed after the payload patches the Preloader) stays for ~8 seconds to connect your tool.

## What's new in v5 (verified on MT6989 rothko)

This is the version confirmed working on the maintainer's rothko (MT6989, engineering/factory preloader): the second Preloader port enumerates, a DA session runs over `mtkclient`/SPFT, and with no tool connected the device boots Android normally after the handshake window.

- **MT6989 rothko target**: `payload/include/target_config.h` now ships the rothko configuration (handshake `0x020585E0`, handler cb `0x0205F604`, `usbdl_vfy_da` `0x02090218`, SLA/DAA/SBC getters `0x02099A50/64/78`), verified against the FACTORY-ROTHKO-0820 debug preloader. Every patch site keeps its expected instruction words, so a mismatched firmware fails safely instead of blind-patching. The OPPO usbEnum latch and the handshake-timeout patch were removed: rothko needs neither (its handshake already waits 2500 ms + 8000 ms).
- **Boot-argument snapshot**: the stock `bl2_ext` entry arguments (x0–x3) are stashed into a BSS buffer at entry (`chainload_boot_args`) and recovered from memory before the trampoline runs. Passing them through x19–x22 was unreliable because `main()` and its callees may clobber those registers.
- **Handshake session save/restore**: the charger-detection cache seeded to open the handshake gate is saved first, every write is verified by read-back, and the original values are restored before chainload. If a restore fails, the payload parks itself (`wfe`) instead of continuing with corrupted Preloader state.
- **Optional SRAM permission relaxation** (off by default): build with `make SRAM_RESTORE=1` to also clear the audited permission fields of the Preloader's SRAM security controllers before the handshake and restore them afterwards. Unrecognized SRAM policies abort safely.
- **8-byte trampoline alignment**: `chainload.S` aligns the trampoline (`EL1 SCTLR.A=1` makes a 4-mod-8 placement data-abort on device).

## Building

Requires an `aarch64-none-elf-gcc` toolchain. `build.sh` downloads one automatically; otherwise override the tools directly:

```bash
./build.sh
# or, with a distro cross-toolchain:
make CC=aarch64-linux-gnu-gcc LD=aarch64-linux-gnu-ld \
     OBJCOPY=aarch64-linux-gnu-objcopy NM=aarch64-linux-gnu-nm
```

Append `SRAM_RESTORE=1` for the optional SRAM variant (see above). The build asserts the payload stays within the 0x20000 `bl2_ext` slot and prints the `chainload_bl2_len` offset used by the injector.

## Injecting

```bash
python3 inject.py bin/lk.img payload/payload.bin patched_lk.bin
```

Flash `patched_lk.bin` to the LK partition and reboot.

Note for rothko: the `bl2_ext` sub-image is covered by CERT1/CERT2 — re-sign it after injecting (e.g. `pwnage24mtk`'s `sign_mtk_cert.py`) or the Preloader will refuse the image.

## Restoring the Android boot (new in this repository)

The original version cannot continue the normal boot. This version builds a **composite image** that carries the stock `bl2_ext`; when the handshake times out (no tool connected), the stock `bl2_ext` is copied back and the normal boot continues — **the device boots normally, no permanent brick**. If a tool connects within the 8 s window and uploads a DA successfully, the flow enters the DA session instead.

Recommended: flash to `lk_a`, keep the stock LK and the B slot as fallback.

## Porting to new firmware

Preloader addresses are firmware-specific — every build can move these functions, so do not copy them across firmwares. Configure the handshake function, callback, patch sites, charger cache, optional SRAM registers, and trampoline window in `payload/include/target_config.h`. Every patch site carries an expected-word check, and every MMIO write is verified by read-back, so a mismatched firmware fails safely instead of blind-patching. Figure out the exact addresses by reverse-engineering your own firmware.

## License

AGPL-3.0-or-later, copyright (C) 2026 R0rt1z2. Includes [`nanoprintf`](https://github.com/charlesnicholson/nanoprintf) (Unlicense / 0BSD dual-licensed).
