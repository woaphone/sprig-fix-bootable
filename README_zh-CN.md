# sprig

<p align="center">
  <img src="images/execution.png" alt="执行流程" width="600">
</p>

English version: [README_en.md](README_en.md) · 原版：[README.md](README.md) · 中文版：README_zh-CN.md

> **署名** —— sprig 原作者：[R0rt1z2](https://github.com/R0rt1z2)。
> 恢复安卓引导（chainload）改造、首个可启动分支：秋逸(逸) &lt;2898684403@qq.com&gt;。
> MT6989（小米 rothko）v5 移植与当前维护：[woaphone](https://github.com/woaphone)（woa手机）。
> 感谢前两位——本仓库建立在他们的工作之上。

## 是什么

基于 [fenrir](https://github.com/R0rt1z2/fenrir) 漏洞的又一个例子：用一段极小的 payload 替换现代 ARMv8 MediaTek 设备 LK 分区里的 `bl2_ext`，在 EL3 下修改 Preloader，关闭 SBC / SLA / DAA 安全校验，从而可以用 `penumbra` 或 `mtkclient` 启动未签名的 DA，任意刷写、导出分区。

你会看到两个 Preloader 端口：第一个约 2 秒消失；第二个（payload 修改后暴露）保持约 8 秒供你连接工具。

## v5 新增（已在 MT6989 rothko 上验证可用）

本版本在维护者的 rothko（MT6989，工程版 preloader）上实测通过：第二 Preloader 端口正常枚举，`mtkclient`/SPFT 可进入 DA 会话；无工具连接时握手超时后设备正常开机。

- **MT6989 rothko 目标配置**：`payload/include/target_config.h` 现附 rothko 配置（握手 `0x020585E0`、回调 `0x0205F604`、`usbdl_vfy_da` `0x02090218`、SLA/DAA/SBC getter `0x02099A50/64/78`），对照 FACTORY-ROTHKO-0820 工程 preloader 逐一核实。每个补丁点都带原始指令校验，固件不匹配会安全停止而不是盲改。去掉了 OPPO usbEnum 闩锁和握手超时补丁：rothko 都不需要（握手本身就等 2500ms + 8000ms）。
- **入口参数快照**：原厂 `bl2_ext` 的入口参数（x0–x3）在入口处存入 BSS（`chainload_boot_args`），跳 trampoline 前从内存恢复。之前经 x19–x22 直传不可靠——`main()` 及其调用方可能改写这些寄存器。
- **握手会话保存/恢复**：为打开握手门而写的充电检测缓存先保存、写入后回读验证，chainload 前恢复原值。任何恢复失败时 payload 原地 `wfe` 停机，绝不带坏状态继续开机。
- **可选 SRAM 权限放宽**（默认关闭）：`make SRAM_RESTORE=1` 会在握手前清掉 Preloader SRAM 安全控制器中经审计的权限字段，事后恢复；遇到不认识的 SRAM 策略直接安全中止。
- **trampoline 8 字节对齐**：`chainload.S` 强制对齐（EL1 `SCTLR.A=1` 下落在 4 mod 8 地址会当场 data abort，实测踩过）。

## 构建

需要 `aarch64-none-elf-gcc` 工具链，脚本会自动下载安装；也可以直接用发行版交叉工具链覆盖：

```bash
./build.sh
# 或使用发行版交叉工具链：
make CC=aarch64-linux-gnu-gcc LD=aarch64-linux-gnu-ld \
     OBJCOPY=aarch64-linux-gnu-objcopy NM=aarch64-linux-gnu-nm
```

可选 SRAM 变体追加 `SRAM_RESTORE=1`（见上）。构建会断言 payload 不超过 0x20000 的 `bl2_ext` 槽位，并打印注入器所需的 `chainload_bl2_len` 偏移。

## 注入

```bash
python3 inject.py bin/lk.img payload/payload.bin patched_lk.bin
```

把 `patched_lk.bin` 刷到 LK 分区，重启即可。

rothko 注意：`bl2_ext` 子镜像受 CERT1/CERT2 覆盖——注入后必须重签（如 `pwnage24mtk` 的 `sign_mtk_cert.py`），否则 Preloader 会拒收。

## 恢复安卓系统引导（本仓库新增）

原版注入后设备无法正常开机。本版本改为**复合镜像**：payload 自带原厂 `bl2_ext`，握手超时（没有工具连接）后自动把原厂 `bl2_ext` 复制回去并继续启动——**设备正常开机，不再变砖**。工具在 8 秒内连接并成功上传 DA 则直接进入 DA 会话。

建议刷 `lk_a`，保留原厂 LK 与 B 槽作回退。

## 移植到新固件

Preloader 地址是版本相关的，每次编译都可能移动，不能跨固件照抄。在 `payload/include/target_config.h` 里配置目标固件的握手函数、回调、补丁点、充电缓存、可选 SRAM 寄存器和 trampoline 窗口即可（每个补丁点都带原始指令校验，每次 MMIO 写入都回读验证，固件不匹配会安全停止）。具体定位方法请自行逆向你的固件。

## 许可证

AGPL-3.0-or-later，版权 (C) 2026 R0rt1z2。含 [`nanoprintf`](https://github.com/charlesnicholson/nanoprintf)（Unlicense / 0BSD 双许可）。
