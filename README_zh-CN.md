# sprig

<p align="center">
  <img src="images/execution.png" alt="执行流程" width="600">
</p>

English version: [README_en.md](README_en.md) · 原版：[README.md](README.md) · 中文版：README_zh-CN.md

> 本版本基于原作者 [R0rt1z2](https://github.com/R0rt1z2) 的 sprig 修改，新增恢复安卓系统引导等功能。
> 修改维护：秋逸(逸) &lt;2898684403@qq.com&gt;

## 是什么

基于 [fenrir](https://github.com/R0rt1z2/fenrir) 漏洞的又一个例子：用一段极小的 payload 替换现代 ARMv8 MediaTek 设备 LK 分区里的 `bl2_ext`，在 EL3 下修改 Preloader，关闭 SBC / SLA / DAA 安全校验，从而可以用 `penumbra` 或 `mtkclient` 启动未签名的 DA，任意刷写、导出分区。

你会看到两个 Preloader 端口：第一个约 2 秒消失；第二个（payload 修改后暴露）保持约 8 秒供你连接工具。

## 构建

需要 `aarch64-none-elf-gcc` 工具链，脚本会自动下载安装：

```bash
./build.sh
```

## 注入

```bash
python3 inject.py bin/lk.img payload/payload.bin patched_lk.bin
```

把 `patched_lk.bin` 刷到 LK 分区，重启即可。

## 恢复安卓系统引导（本仓库新增）

原版注入后设备无法正常开机。本版本改为**复合镜像**：payload 自带原厂 `bl2_ext`，握手超时（没有工具连接）后自动把原厂 `bl2_ext` 复制回去并继续启动——**设备正常开机，不再变砖**。工具在 8 秒内连接并成功上传 DA 则直接进入 DA 会话。

建议刷 `lk_a`，保留原厂 LK 与 B 槽作回退。

## 移植到新固件

Preloader 地址是版本相关的，每次编译都可能移动，不能跨固件照抄。在 `payload/include/target_config.h` 里配置目标固件的握手函数、回调、补丁点、USB 闩锁和 trampoline 窗口即可（每个补丁点都带原始指令校验，固件不匹配会安全停止）。具体定位方法请自行逆向你的固件。

## 许可证

AGPL-3.0-or-later，版权 (C) 2026 R0rt1z2。含 [`nanoprintf`](https://github.com/charlesnicholson/nanoprintf)（Unlicense / 0BSD 双许可）。
