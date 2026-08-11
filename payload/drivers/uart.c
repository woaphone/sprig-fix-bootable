/* Modified by 秋逸(逸) <2898684403@qq.com> */
#include <mmio.h>
#include <uart.h>

static inline void uart_write(uint32_t offset, uint32_t value)
{
    writel(value, MTK_UART_BASE + offset);
}

static inline uint32_t uart_read(uint32_t offset)
{
    return readl(MTK_UART_BASE + offset);
}

void mtk_uart_putc(int ch)
{
    /*
     * Bounded busy-wait: if the UART base is wrong (firmware build
     * change), fall through instead of hanging the payload forever.
     */
    for (int i = 0; i < 10000; i++) {
        if (uart_read(MTK_UART_LSR) & MTK_UART_LSR_THRE)
            break;
    }

    uart_write(MTK_UART_THR, ch);
}
