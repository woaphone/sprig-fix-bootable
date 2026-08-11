# sprig

<p align="center">
  <img src="images/execution.png" alt="Execution flow" width="600">
</p>

English version: README_en.md · 中文版：[README_zh-CN.md](README_zh-CN.md) · Original author's version: [README.md](README.md)

> This version is a fork of [R0rt1z2](https://github.com/R0rt1z2)'s sprig, with the Android boot-restore (chainload) feature added.
> Modified and maintained by 秋逸(逸) &lt;2898684403@qq.com&gt;

## What

Yet another example built on the underlying vulnerability used in [fenrir](https://github.com/R0rt1z2/fenrir): a tiny payload replaces the `bl2_ext` image in the LK partition of modern ARMv8 MediaTek devices, runs in EL3, and patches the Preloader in memory to disable SBC / SLA / DAA checks. This allows booting unsigned DAs with `penumbra` or `mtkclient` for unrestricted flash/dump operations.

You will see two Preloader ports: the first disappears within ~2 seconds, the second (exposed after the payload patches the Preloader) stays for ~8 seconds to connect your tool.

## Building

Requires `aarch64-none-elf-gcc`; the script downloads it automatically:

```bash
./build.sh
```

## Injecting

```bash
python3 inject.py bin/lk.img payload/payload.bin patched_lk.bin
```

Flash `patched_lk.bin` to the LK partition and reboot.

## Restoring the Android boot (new in this repository)

The original version cannot continue the normal boot. This version builds a **composite image** that carries the stock `bl2_ext`; when the handshake times out (no tool connected), the stock `bl2_ext` is copied back and the normal boot continues — **the device boots normally, no permanent brick**. If a tool connects within the 8 s window and uploads a DA successfully, the flow enters the DA session instead.

Recommended: flash to `lk_a`, keep the stock LK and the B slot as fallback.

## Porting to new firmware

Preloader addresses are firmware-specific — every build can move these functions, so do not copy them across firmwares. Configure the handshake function, callback, patch sites, USB latch, and trampoline window in `payload/include/target_config.h`. Every patch site carries an expected-word check, so a mismatched firmware fails safely instead of blind-patching. Figure out the exact addresses by reverse-engineering your own firmware.

## License

AGPL-3.0-or-later, copyright (C) 2026 R0rt1z2. Includes [`nanoprintf`](https://github.com/charlesnicholson/nanoprintf) (Unlicense / 0BSD dual-licensed).
