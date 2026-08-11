/* Created by 秋逸(逸) <2898684403@qq.com> */
/*
 * Position-independent chainload trampoline, structurally identical to
 * the working reference payload's trampoline.
 *
 * Installed by chainload_to_bl2() at the per-target trampoline address.
 *
 * Entry args:
 *   x0-x3 = original bl2_ext entry arguments (passed straight through)
 *   x4    = src (original bl2_ext copy)
 *   x5    = dst (0xB8000000)
 *   x6    = len (original bl2_ext length, patched at build time)
 *
 * Forward overlapping copy (dst < src), then cache maintenance, then
 * branch to the destination (now the original bl2_ext).
 */
.section .text.chainload
.global chainload_trampoline
chainload_trampoline:
    mov     x10, x5
    mov     x11, x6
1:  cbz     x6, 2f
    ldr     x9, [x4], 8
    str     x9, [x5], 8
    sub     x6, x6, 8
    b       1b
2:  /* clean D-cache over the destination range, 64-byte aligned */
    and     x12, x10, 0xffffffffffffffc0
    add     x13, x10, x11
    add     x13, x13, 0x3f
    and     x13, x13, 0xffffffffffffffc0
3:  cmp     x12, x13
    b.hs    4f
    dc      cvac, x12
    add     x12, x12, 0x40
    b       3b
4:  dsb     sy
    ic      ialluis
    dsb     sy
    isb
    br      x10
.global chainload_trampoline_end
chainload_trampoline_end:
.end
