/* Created by 秋逸(逸) <2898684403@qq.com> */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <target.h>

/*
 * Per-target safe window for the chainload trampoline, set by the
 * target config (verified against that firmware's bl2_ext layout).
 */
#define CHAINLOAD_TRAMP_ADDR   TRAMPOLINE_ADDR
#define CHAINLOAD_TRAMP_SIZE   0x100UL

void chainload_to_bl2(void);
