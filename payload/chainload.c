/* Created by 秋逸(逸) <2898684403@qq.com> */
#include <chainload.h>
#include <debug.h>
#include <mmio.h>

extern unsigned char chainload_trampoline[];
extern unsigned char chainload_trampoline_end[];

/* First symbol in .data, patched by inject.py with the original bl2_ext
   length. See the linker script. */
extern uint64_t chainload_bl2_len;

static void chainload_copy8(uint64_t *dst, const uint64_t *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
        dst[i] = src[i];
}

void chainload_to_bl2(void)
{
    uintptr_t tramp = (uintptr_t)chainload_trampoline;
    size_t tsize = (uintptr_t)chainload_trampoline_end - (uintptr_t)chainload_trampoline;
    uintptr_t safe = CHAINLOAD_TRAMP_ADDR;
    uint64_t len = chainload_bl2_len;

    if (len == 0 || (len & 7) != 0) {
        while (1);
    }

    if (tsize > CHAINLOAD_TRAMP_SIZE) {
        while (1);
    }

    chainload_copy8((uint64_t *)safe, (uint64_t *)tramp, (tsize + 7) / 8);

    flush_dcache_range(safe, CHAINLOAD_TRAMP_SIZE);
    invalidate_icache_range(safe, CHAINLOAD_TRAMP_SIZE);

    /*
     * Pass the original entry arguments (x19-x22) straight through as
     * x0-x3, with the copy parameters in x4/x5/x6 - exactly like the
     * working reference payload.
     */
    __asm__ volatile(
        "mov x0, x19\n"
        "mov x1, x20\n"
        "mov x2, x21\n"
        "mov x3, x22\n"
        "mov x4, %[src]\n"
        "mov x5, %[dst]\n"
        "mov x6, %[len]\n"
        "br  %[tramp]\n"
        :: [src] "r" (0xB8020000UL), [dst] "r" (0xB8000000UL),
           [len] "r" (len), [tramp] "r" (safe)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6");
}
