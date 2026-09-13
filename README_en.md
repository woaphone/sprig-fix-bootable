# sprig

<p align="center">
  <img src="images/execution.png" alt="Execution flow" width="600">
</p>

English version: README_en.md · 中文版：[README_zh-CN.md](README_zh-CN.md) · Original author's version: [README.md](README.md)

> This version is a fork of [R0rt1z2](https://github.com/R0rt1z2)'s sprig. Making it bootable (the Android boot-restore / chainload rework) is 秋逸(逸)'s work &lt;2898684403@qq.com&gt;; [woaphone](https://github.com/woaphone) only added MT6989 (Xiaomi rothko) support — added because the original sprig cannot successfully send a Download Agent on MT6989.

## What

Yet another example built on the underlying vulnerability used in [fenrir](https://github.com/R0rt1z2/fenrir): a tiny payload replaces the `bl2_ext` image in the LK partition of modern ARMv8 MediaTek devices, runs in EL3, and patches the Preloader in memory to disable SBC / SLA / DAA checks. This allows booting unsigned DAs with `penumbra` or `mtkclient` for unrestricted flash/dump operations.

You will see two Preloader ports: the first disappears within ~2 seconds, the second (exposed after the payload patches the Preloader) stays for ~8 seconds to connect your tool.

## Acknowledgments

- [R0rt1z2](https://github.com/R0rt1z2) — author of the original sprig.
- 秋逸(逸) &lt;2898684403@qq.com&gt; — made it bootable: the Android boot-restore (chainload) rework and the first bootable fork.

Thank you both — this repository would not exist without your work. (The MT6989 support in this fork was added by woaphone only because the original sprig could not send a Download Agent on MT6989.)

## What's new in v5 (verified on MT6989 rothko)

**Why v5 exists:** on MT6989, the original sprig — including the boot-restore fork — cannot successfully send a Download Agent. The Preloader gates its handshake behind a charger-detection cache, so without touching it the second port never even appears; and even when a DA session starts, the payload leaving modified Preloader session state behind ends the session with `All storage init fail`. v5 seeds that cache with save/read-back/restore around the handshake (plus the optional SRAM permission relaxation), which is what makes a DA actually run.

This is the version confirmed working on the maintainer's rothko (MT6989, engineering/factory preloader): the second Preloader port enumerates, a DA session runs over `mtkclient`/SPFT, and with no tool connected the device boots Android normally after the handshake window.

- **MT6989 rothko target**: `payload/include/target_config.h` now ships the rothko configuration (handshake `0x020585E0`, handler cb `0x0205F604`, `usbdl_vfy_da` `0x02090218`, SLA/DAA/SBC getters `0x02099A50/64/78`), verified against the FACTORY-ROTHKO-0820 debug preloader. Every patch site keeps its expected instruction words, so a mismatched firmware fails safely instead of blind-patching. The OPPO usbEnum latch and the handshake-timeout patch were removed: rothko needs neither (its handshake already waits 2500 ms + 8000 ms).
- **Boot-argument snapshot**: the stock `bl2_ext` entry arguments (x0–x3) are stashed into a BSS buffer at entry (`chainload_boot_args`) and recovered from memory before the trampoline runs. Passing them through x19–x22 was unreliable because `main()` and its callees may clobber those registers.
- **Handshake session save/restore**: the charger-detection cache seeded to open the handshake gate is saved first, every write is verified by read-back, and the original values are restored before chainload. If a restore fails, the payload parks itself (`wfe`) instead of continuing with corrupted Preloader state.
- **Optional SRAM permission relaxation** (off by default): build with `make SRAM_RESTORE=1` to also clear the audited permission fields of the Preloader's SRAM security controllers before the handshake and restore them afterwards. Unrecognized SRAM policies abort safely.
- **8-byte trampoline alignment**: `chainload.S` aligns the trampoline (`EL1 SCTLR.A=1` makes a 4-mod-8 placement data-abort on device).

## Everything that changed compared to upstream sprig (and why)

Baseline: R0rt1z2's sprig as of this fork — an MT6991/Pacman example whose payload *replaces* `bl2_ext` and never returns to the normal boot.

### Boot restore (the core feature)

- **Composite bl2_ext image** (`inject.py`): upstream swapped the `bl2_ext` sub-partition for the payload, so after injecting, the device could not boot normally ("will remain 'bricked'" per the upstream README). This fork injects `[payload padded to 0x20000][stock bl2_ext]` and patches the stock length into `chainload_bl2_len` (first `.data` symbol; its offset is exported by the build) — the payload carries its own escape path.
- **`chainload.c` / `chainload.S` / `chainload.h` (new)**: when the handshake window ends without a DA session, a position-independent trampoline copies the stock `bl2_ext` back over the payload (forward copy, dcache clean + icache invalidate) and jumps into it with the original entry arguments — normal boot continues instead of bricking. The trampoline lands in a per-target safe window (`TRAMPOLINE_ADDR`) past the composite image, and is 8-byte aligned because EL1 runs with `SCTLR.A=1` (a 4-mod-8 placement data-aborted on device).
- **`entry.S`**: the stock `bl2_ext` entry arguments (x0–x3) are stashed into a BSS snapshot (`chainload_boot_args`) and recovered from memory before the trampoline. Passing them through x19–x22 was unreliable once `main()` started using callee-saved registers; upstream never preserved the arguments because it never intended to return.
- **`linker.ld`**: added a MEMORY region, a 64 KB stack and an `ASSERT` that keeps the payload inside the 0x20000 `bl2_ext` slot; the load address is per-target (upstream hardcoded 0x62F00000).

### Fail-safe patching

- **`payload/include/target_config.h` (new)**: all per-target addresses in one file (upstream hardcoded MT6991 addresses inside `main.c` / `patches.c` / `bldr.c`); `target.h` simply includes it.
- **`patches.c` / `patches.h`**: every patch site now carries the expected original instruction words (`PATCH_*_CHECKED`); `patch_apply_all()` returns a status and `main()` refuses to continue on mismatch — a different preloader build fails safely instead of being blind-patched.
- **Patch set**: upstream patched the MT6991 handshake timeout, the UART-log switch, AEE boot and the direct SBC/SLA/DAA check functions. The rothko set is four patches — `usbdl_vfy_da` (accept any DA) plus the SLA/DAA/SBC security-flag getters (`mov w0,#0; ret`). The timeout patch is unnecessary on rothko (its handshake already waits 2500 ms + 8000 ms) and the AEE patch was not needed.

### Handshake session handling

- **`bldr.c` / `bldr.h` / `main.c`**: upstream just called the handshake and never came back. This fork seeds the charger-detection cache that gates the handshake (otherwise it prints "PMIC not dectect usb cable!" and returns immediately), saving the original values first and verifying every write by read-back. When the handshake returns with no DA session, the original session state is restored before chainload; if a restore fails, the payload parks itself (`wfe`) instead of booting with corrupted Preloader state (`BLDR_ERR_RESTORE`).
- **Optional SRAM permission relaxation** (`make SRAM_RESTORE=1`, default off): clears the audited permission fields of the Preloader's SRAM security controllers before the handshake and restores them afterwards; unrecognized SRAM policies abort safely.
- **`main.c`**: dropped upstream's `set_log_switch(LOG_ON)` and the fixed 5-second wait call (both MT6991-specific addresses); the OPPO usbEnum-latch clear is conditional and compiled out on rothko.

### Console / driver

- **`debug.c`**: the nanoprintf UART console was replaced with a no-op `printf` stub, matching the silent profile of the working reference payload; call sites and string literals are kept so the code layout stays close to the reference.
- **`drivers/uart.c` / `uart.h`**: UART base corrected to the Preloader's `uart_base` pointer value (a SoC property, not a per-build address) and the transmit busy-wait is bounded so a wrong base cannot hang the payload.

### Removed

- **`hooks.c` / `heap.c`**: upstream's heap-hook trampoline framework and free-list dumper were research tooling for the original bug; this payload drives the PL download path directly and needs neither (it also keeps the payload small). The headers are kept but unused.
- **`extract.sh` and the `bin/` sample firmware blobs**: extraction moved to the pwnage24mtk tooling (this device's `lk` is a five-sub-image V6+AVF container), and the MT6991 sample binaries should not be redistributed here.

### Build & packaging

- **`Makefile`**: toolchain commands overridable from the command line (distro cross-toolchains), `-MMD -MP` dependency tracking plus forced rebuilds so feature flags never reuse stale objects, and export of `bl2_len_offset.txt` (the address of `chainload_bl2_len`) consumed by the injector.
- **`inject.py`**: composite injection as described above; also documents that the `bl2_ext` sub-image is covered by CERT1/CERT2 and must be re-signed after injecting.
- **READMEs** (English / Chinese / the fun one) documenting all of the above.

## What changed compared to 秋逸's bootable fork (and why)

Baseline: the fork as received from 秋逸(逸) — chainload anti-brick, expected-word patches, and a *sample* MT6991 `target_config.h`. It had never been brought up on MT6989; on rothko it could not send a Download Agent at all. Everything below is what this fork added on top, and the combination is what was verified working on rothko.

- **Real MT6989 target configuration** (`target_config.h`, `linker.ld`, `inject.py`): replaced the MT6991 example values with addresses reverse-engineered from rothko's FACTORY-ROTHKO-0820 engineering preloader (handshake `0x020585E0`, handler cb `0x0205F604`, `usbdl_vfy_da` `0x02090218`, SLA/DAA/SBC getters `0x02099A50/64/78`) and moved the bl2_ext window `0xB8000000` → `0x78000000` (rothko's `system_bl2-ext` reservation). Without this, the fork pointed at functions that simply do not exist in the rothko preloader.
- **Charger-detection cache seeding** (`bldr.c`): rothko gates its handshake behind a charger-type cache; without seeding it, the handshake prints "PMIC not dectect usb cable!" and returns immediately — the second port never appears and no DA can ever be sent. The sample config had no equivalent.
- **Handshake session save / read-back / restore** (`bldr.c`, `bldr.h`, `main.c`): the received `bldr_handshake()` just called the preloader handshake. Now the session state (charger cache, and optionally the SRAM security-controller permissions) is saved first, every write is verified by read-back, and everything is restored before chainload; on restore failure the payload returns `BLDR_ERR_RESTORE` and `main()` parks itself (`wfe`) instead of booting with corrupted Preloader state. This is what turned "DA session starts but ends in `All storage init fail`" into a working DA.
- **Boot-argument snapshot** (`entry.S`, `chainload.c`): the received chainload passed the stock `bl2_ext` entry arguments through x19–x22, assuming `main()` never touches callee-saved registers. On the MT6989 build it does, so the stock `bl2_ext` received garbage on restore; the arguments now go through a BSS snapshot (`chainload_boot_args`).
- **8-byte trampoline alignment** (`chainload.S`): `.align 3` — on device the trampoline landed at a 4-mod-8 address and EL1 (`SCTLR.A=1`) data-aborted.
- **Conditional OPPO usbEnum latch** (`main.c`): the unconditional `writeb(0, OPPO_USB_ENUM_LOCK)` does not compile for a target that does not define the macro (rothko has no such latch); it is now `#ifdef`-guarded.
- **Dropped the handshake-timeout patch**: rothko's handshake already waits 2500 ms + 8000 ms (`w28=0x9C4 / w23=0x1F40`), so `PATCH_TIMEOUT_*` is unnecessary on this target.
- **Optional SRAM permission relaxation** (`make SRAM_RESTORE=1`, default off): clears the audited permission fields of the Preloader's SRAM security controllers before the handshake and restores them afterwards; unrecognized SRAM policies abort safely.
- **Build tooling** (`Makefile`): `SRAM_RESTORE` flag, `-MMD -MP` dependency tracking plus forced rebuilds so flag changes never reuse stale objects, and the bl2_len offset base moved to `0x78000000`.

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
