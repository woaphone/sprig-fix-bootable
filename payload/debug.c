/* Modified by 秋逸(逸) <2898684403@qq.com> */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/*
 * No-op console, mirroring the working reference payload: its print
 * function was a `mov w0, #0; ret` stub. Callers and string literals
 * are kept so the layout stays close to the reference; nothing is
 * emitted on UART.
 */

int printf(const char* fmt, ...)
{
    (void)fmt;
    return 0;
}
