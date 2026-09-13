.section .text.entry
.global _start

_start:
    mov     x19, x0
    mov     x20, x1
    mov     x21, x2
    mov     x22, x3
    ldr     x0, =__stack_end
    mov     sp, x0
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x2, #0
1:
    cmp     x0, x1
    b.ge    2f
    str     x2, [x0], #8
    b       1b
2:
    /* Keep entry arguments valid even when main uses callee-saved registers. */
    adrp    x8, chainload_boot_args
    add     x8, x8, :lo12:chainload_boot_args
    stp     x19, x20, [x8]
    stp     x21, x22, [x8, #16]
    bl      main
3:
    wfe
    b       3b

.section .bss.entry_args,"aw",%nobits
.balign 16
.global chainload_boot_args
.type chainload_boot_args, %object
chainload_boot_args:
    .skip 32
.size chainload_boot_args, .-chainload_boot_args
