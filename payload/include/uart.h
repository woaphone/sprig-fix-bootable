/* Modified by 秋逸(逸) <2898684403@qq.com> */
#pragma once

#include <stdint.h>

/*
 * MT6991 UART base, taken from the preloader's uart_base pointer
 * (0x4016000000 stored at 0x21093C8). This is a SoC property, not a
 * per-build address.
 */
#define MTK_UART_BASE       0x4016000000

#define MTK_UART_THR        0x00
#define MTK_UART_LSR        0x14

#define MTK_UART_LSR_THRE   (1 << 5)

void mtk_uart_putc(int ch);
