# sprig

<p align="center">
  <img src="images/execution.png" alt="执行流程" width="600">
</p>

English version: [README_en.md](README_en.md) · 原版：[README.md](README.md) · 中文版：README_zh-CN.md

> 本版本基于 [R0rt1z2](https://github.com/R0rt1z2) 的 sprig。让它能启动（恢复安卓引导 / chainload 改造）是秋逸(逸) &lt;2898684403@qq.com&gt; 的工作；[woaphone](https://github.com/woaphone) 只是添加了 MT6989（小米 rothko）支持——起因是原版 sprig 在 MT6989 上发不了 Download Agent。

## 是什么

基于 [fenrir](https://github.com/R0rt1z2/fenrir) 漏洞的又一个例子：用一段极小的 payload 替换现代 ARMv8 MediaTek 设备 LK 分区里的 `bl2_ext`，在 EL3 下修改 Preloader，关闭 SBC / SLA / DAA 安全校验，从而可以用 `penumbra` 或 `mtkclient` 启动未签名的 DA，任意刷写、导出分区。

你会看到两个 Preloader 端口：第一个约 2 秒消失；第二个（payload 修改后暴露）窗口很短（MT6989 实测约 2 秒）供你连接工具——v5 还额外把它推迟了 1.5 秒，方便主机工具抓到。

## 致谢

- [R0rt1z2](https://github.com/R0rt1z2) —— sprig 原作者。
- 秋逸(逸) &lt;2898684403@qq.com&gt; —— 让它能启动：恢复安卓引导（chainload）改造与首个可启动分支。

感谢二位——没有你们的工作就没有这个仓库。（本 fork 的 MT6989 支持由 woaphone 添加，起因只是原版 sprig 在 MT6989 上发不了 Download Agent。）

## v5 新增（已在 MT6989 rothko 上验证可用）

**v5 的由来**：在 MT6989 上，原版 sprig——包括恢复引导的可启动 fork——无法成功发送 Download Agent。Preloader 的握手被充电检测缓存门控，不动它第二端口根本不会出现；即使 DA 会话开始，payload 留下被改动的 Preloader 会话状态也会让会话以 `All storage init fail` 告终。v5 在握手前后对这份会话状态做保存/回读验证/恢复（外加可选的 SRAM 权限放宽），DA 才真正跑得起来。

本版本在维护者的 rothko（MT6989，工程版 preloader）上实测通过：第二 Preloader 端口正常枚举，`mtkclient`/SPFT 可进入 DA 会话；无工具连接时握手超时后设备正常开机。

- **MT6989 rothko 目标配置**：`payload/include/target_config.h` 现附 rothko 配置（握手 `0x020585E0`、回调 `0x0205F604`、`usbdl_vfy_da` `0x02090218`、SLA/DAA/SBC getter `0x02099A50/64/78`），对照 FACTORY-ROTHKO-0820 工程 preloader 逐一核实。每个补丁点都带原始指令校验，固件不匹配会安全停止而不是盲改。去掉了 OPPO usbEnum 闩锁和握手超时补丁：rothko 都不需要（握手本身就等 2500ms + 8000ms）。
- **入口参数快照**：原厂 `bl2_ext` 的入口参数（x0–x3）在入口处存入 BSS（`chainload_boot_args`），跳 trampoline 前从内存恢复。之前经 x19–x22 直传不可靠——`main()` 及其调用方可能改写这些寄存器。
- **握手会话保存/恢复**：为打开握手门而写的充电检测缓存先保存、写入后回读验证，chainload 前恢复原值。任何恢复失败时 payload 原地 `wfe` 停机，绝不带坏状态继续开机。
- **可选 SRAM 权限放宽**（默认关闭）：`make SRAM_RESTORE=1` 会在握手前清掉 Preloader SRAM 安全控制器中经审计的权限字段，事后恢复；遇到不认识的 SRAM 策略直接安全中止。
- **trampoline 8 字节对齐**：`chainload.S` 强制对齐（EL1 `SCTLR.A=1` 下落在 4 mod 8 地址会当场 data abort，实测踩过）。
- **第二端口推迟 1.5 秒**：原版行为下，端口在 USB 枚举完成后（握手开始约 400ms）立刻出现，而工具监听窗（2500ms，`w28=0x9C4`，从握手入口起算）约 2 秒后就关死——主机工具很难抓到。现在 payload 先等 1.5 秒（`target_config.h` 的 `BLDR_HANDSHAKE_DELAY_US`，调用 Preloader 自带的 `udelay` `0x020815AC`）再进握手，端口出现时工具早已在监听。旧文档说的"约 8 秒窗口"其实是 USB 枚举超时（`w23=0x1F40`），不是工具窗口。

## 相对原版 sprig 的全部改动（及原因）

基线：R0rt1z2 的 sprig 在本 fork 分出时的状态——MT6991/Pacman 示例，payload **替换** `bl2_ext` 且从不回到正常启动。

### 启动恢复（核心功能）

- **复合 bl2_ext 镜像**（`inject.py`）：原版直接用 payload 顶掉 `bl2_ext` 子分区，注入后设备无法正常开机（原版 README 原话 "will remain 'bricked'"）。本版注入 `[payload 补齐到 0x20000][原厂 bl2_ext]`，并把原厂长度回填进 `chainload_bl2_len`（`.data` 首符号，偏移由构建导出）——payload 自带退路。
- **`chainload.c` / `chainload.S` / `chainload.h`（新增）**：握手窗口内没有 DA 会话时，位置无关的 trampoline 把原厂 `bl2_ext` 拷回 payload 位置（前向拷贝，dcache clean + icache invalidate），带上原始入口参数跳过去——正常开机继续，不变砖。trampoline 落在按目标配置的安全窗口（`TRAMPOLINE_ADDR`，复合镜像之后），且 8 字节对齐——EL1 开着 `SCTLR.A=1`，落在 4 mod 8 地址实测当场 data abort。
- **`entry.S`**：原厂 `bl2_ext` 的入口参数（x0–x3）在入口存进 BSS 快照（`chainload_boot_args`），跳 trampoline 前从内存恢复。`main()` 开始使用 callee-saved 寄存器后，经 x19–x22 直传不可靠；原版从不保存参数，因为它从没打算返回。
- **`linker.ld`**：新增 MEMORY 区域、64 KB 栈和把 payload 限制在 0x20000 `bl2_ext` 槽位内的 `ASSERT`；加载地址按目标配置（原版硬编码 0x62F00000）。

### 补丁防呆

- **`payload/include/target_config.h`（新增）**：所有目标相关地址集中在一个文件（原版把 MT6991 地址硬编码在 `main.c` / `patches.c` / `bldr.c` 里）；`target.h` 只负责包含它。
- **`patches.c` / `patches.h`**：每个补丁点都带期望的原始指令字（`PATCH_*_CHECKED`）；`patch_apply_all()` 返回状态，`main()` 校验失败即拒绝继续——换一版 preloader 会安全停止，而不是被盲改。
- **补丁集**：原版补 MT6991 的握手超时、UART 日志开关、AEE boot 和直连的 SBC/SLA/DAA 校验函数。rothko 集合是四个补丁——`usbdl_vfy_da`（放行任意 DA）加 SLA/DAA/SBC 安全标志 getter（`mov w0,#0; ret`）。超时补丁在 rothko 上不需要（握手本身就等 2500ms + 8000ms），AEE 补丁也用不上。

### 握手会话处理

- **`bldr.c` / `bldr.h` / `main.c`**：原版调完握手就再也没回来。本版先写握手门控所需的充电检测缓存（否则打印 "PMIC not dectect usb cable!" 直接返回），写之前保存原值、每次写入回读验证；握手返回（无 DA 会话）后先恢复原会话状态再 chainload。任何恢复失败时 payload 原地 `wfe` 停机（`BLDR_ERR_RESTORE`），绝不带坏掉的 Preloader 状态开机。
- **可选 SRAM 权限放宽**（`make SRAM_RESTORE=1`，默认关闭）：握手前清掉 Preloader SRAM 安全控制器中经审计的权限字段，事后恢复；遇到不认识的 SRAM 策略安全中止。
- **`main.c`**：去掉原版的 `set_log_switch(LOG_ON)` 和固定 5 秒等待调用（都是 MT6991 专属地址）；OPPO usbEnum 闩锁清除改为条件编译，rothko 上不参与编译。

### 控制台 / 驱动

- **`debug.c`**：nanoprintf UART 控制台换成 no-op `printf` 桩，与参考 payload 的静默行为一致；调用点和字符串保留，代码布局尽量贴近参考实现。
- **`drivers/uart.c` / `uart.h`**：UART 基址改为 Preloader `uart_base` 指针里的值（SoC 属性，不随固件版本变）；发送忙等待加上限，基址不对也不会挂死 payload。

### 删除

- **`hooks.c` / `heap.c`**：原版的堆 hook trampoline 框架和 free list 导出是针对原始漏洞的调试工具；本 payload 直连 PL 下载路径，两者都用不上（也顺便控制体积）。头文件保留但已不使用。
- **`extract.sh` 和 `bin/` 下的示例固件**：提取流程改用 pwnage24mtk（这台设备的 `lk` 是五子镜像 V6+AVF 容器），MT6991 示例二进制也不应再随仓库分发。

### 构建与打包

- **`Makefile`**：工具链命令可从命令行覆盖（发行版交叉工具链）；`-MMD -MP` 依赖跟踪加强制重建，特性开关永不复用旧目标文件；导出 `bl2_len_offset.txt`（`chainload_bl2_len` 地址）供注入器使用。
- **`inject.py`**：如上述的复合注入；并注明 `bl2_ext` 子镜像受 CERT1/CERT2 覆盖，注入后必须重签。
- **README**（英文 / 中文 / 八股文）记录以上全部内容。

## 相较秋逸(逸)的可启动 fork 改了什么（及为什么）

基线：从秋逸(逸)手里接过的 fork——已有 chainload 防砖、期望字补丁校验和一份 *示例* MT6991 `target_config.h`。它在 MT6989 上从未点亮过；在 rothko 上根本发不了 Download Agent。下面是本 fork 在其基础上新增的全部内容，这些叠加起来才是在 rothko 上验证可用的版本。

- **真实的 MT6989 目标配置**（`target_config.h`、`linker.ld`、`inject.py`）：把 MT6991 示例值换成从 rothko FACTORY-ROTHKO-0820 工程 preloader 逆向出的实测地址（握手 `0x020585E0`、回调 `0x0205F604`、`usbdl_vfy_da` `0x02090218`、SLA/DAA/SBC getter `0x02099A50/64/78`），bl2_ext 窗口从 `0xB8000000` 移到 `0x78000000`（rothko 的 `system_bl2-ext` 保留区）。不改这个，fork 指向的函数在 rothko preloader 里根本不存在。
- **充电检测缓存种子**（`bldr.c`）：rothko 的握手被充电类型缓存门控，不写它握手就打印 "PMIC not dectect usb cable!" 立即返回——第二端口永远不会出现，任何 DA 都发不出去。示例配置里没有对应项。
- **握手会话保存 / 回读 / 恢复**（`bldr.c`、`bldr.h`、`main.c`）：原来的 `bldr_handshake()` 就是裸调 preloader 握手。现在先把会话状态（充电缓存，可选的 SRAM 安全控制器权限）保存起来，每次写入回读验证，chainload 前全部恢复原值；恢复失败返回 `BLDR_ERR_RESTORE`，`main()` 原地 `wfe` 停机，绝不带坏掉的 Preloader 状态开机。这一步把"DA 会话能开始但以 `All storage init fail` 告终"变成了真正能用的 DA。
- **入口参数快照**（`entry.S`、`chainload.c`）：原版 chainload 经 x19–x22 直传原厂 `bl2_ext` 的入口参数，前提是 `main()` 不碰 callee-saved 寄存器——MT6989 版编译出来偏偏会碰，导致恢复启动时原厂 `bl2_ext` 收到的是垃圾参数；现在参数改走 BSS 快照（`chainload_boot_args`）。
- **trampoline 8 字节对齐**（`chainload.S`）：加 `.align 3`——上机时 trampoline 落在 4 mod 8 地址，EL1（`SCTLR.A=1`）当场 data abort。
- **OPPO usbEnum 闩锁改条件编译**（`main.c`）：无条件的 `writeb(0, OPPO_USB_ENUM_LOCK)` 在没定义该宏的目标上编不过（rothko 没有这个闩锁），现在包在 `#ifdef` 里。
- **去掉握手超时补丁**：rothko 的握手本身就等 2500ms + 8000ms（`w28=0x9C4 / w23=0x1F40`），`PATCH_TIMEOUT_*` 在这个目标上用不着。
- **可选 SRAM 权限放宽**（`make SRAM_RESTORE=1`，默认关闭）：握手前清掉 Preloader SRAM 安全控制器中经审计的权限字段，事后恢复；遇到不认识的 SRAM 策略安全中止。
- **构建工具**（`Makefile`）：`SRAM_RESTORE` 开关、`-MMD -MP` 依赖跟踪加强制重建（特性开关永不复用旧目标文件）、bl2_len 偏移基准改为 `0x78000000`。
- **第二端口推迟 1.5 秒**（`bldr.c`、`target_config.h`）：接手时是直接进握手，usbdl 端口枚举完就出现、2500ms 监听窗约 2 秒后关死，主机工具很难抓。现在 payload 通过 Preloader 的 `udelay` 先等 `BLDR_HANDSHAKE_DELAY_US`（rothko 上 1.5 秒）再进握手，端口出现时工具已在监听；会话保存/恢复与 chainload 逻辑一行未动。

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

原版注入后设备无法正常开机。本版本改为**复合镜像**：payload 自带原厂 `bl2_ext`，握手超时（没有工具连接）后自动把原厂 `bl2_ext` 复制回去并继续启动——**设备正常开机，不再变砖**。工具在监听窗口内连接并成功上传 DA 则直接进入 DA 会话。

建议刷 `lk_a`，保留原厂 LK 与 B 槽作回退。

## 移植到新固件

Preloader 地址是版本相关的，每次编译都可能移动，不能跨固件照抄。在 `payload/include/target_config.h` 里配置目标固件的握手函数、回调、补丁点、充电缓存、可选 SRAM 寄存器和 trampoline 窗口即可（每个补丁点都带原始指令校验，每次 MMIO 写入都回读验证，固件不匹配会安全停止）。具体定位方法请自行逆向你的固件。

## 许可证

AGPL-3.0-or-later，版权 (C) 2026 R0rt1z2。含 [`nanoprintf`](https://github.com/charlesnicholson/nanoprintf)（Unlicense / 0BSD 双许可）。
