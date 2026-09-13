#include <chainload.h>
#include <debug.h>
#include <mmio.h>

extern unsigned char chainload_trampoline[];
extern unsigned char chainload_trampoline_end[];

/* First symbol in .data, patched by inject.py with the original bl2_ext
   length. See the linker script. */
extern uint64_t chainload_bl2_len;
extern uint64_t chainload_boot_args[4];

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
     * main and its callees may use x19-x22. Recover the original arguments
     * from the entry snapshot before the trampoline overwrites the payload.
     */
    __asm__ volatile(
        "ldp x0, x1, [%[args]]\n"
        "ldp x2, x3, [%[args], #16]\n"
        "mov x4, %[src]\n"
        "mov x5, %[dst]\n"
        "mov x6, %[len]\n"
        "br  %[tramp]\n"
        :: [args] "r" (chainload_boot_args),
           [src] "r" (0x78020000UL), [dst] "r" (0x78000000UL),
           [len] "r" (len), [tramp] "r" (safe)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "memory");
}
