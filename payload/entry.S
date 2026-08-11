/* Modified by 秋逸(逸) <2898684403@qq.com> */
.section .text.entry
.global _start

_start:
    /* The original bl2_ext entry ABI hands us x0-x3. The payload runs
       first, so stash them in x19-x22 (callee-saved, never touched by
       the copy trampoline) and pass them straight through on
       chainload. This mirrors the working reference payload. */
    mov     x19, x0
    mov     x20, x1
    mov     x21, x2
    mov     x22, x3

    ldr     x0, =__stack_end
    mov     sp, x0

	/* Zero BSS section */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x2, #0
1:
    cmp     x0, x1
    b.ge    2f
    str     x2, [x0], #8
    b       1b
2:
    bl      main

3:  
    wfe
    b       3b
