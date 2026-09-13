ENTRY(_start)

MEMORY
{
    /* The preloader loads the composite bl2_ext image at 0x78000000
       (res_mem_info "system_bl2-ext" on MT6989 rothko). The payload
       itself must stay below 0x78020000; the original bl2_ext is
       appended there by the injector. */
    DRAM (rwx) : ORIGIN = 0x78000000, LENGTH = 0x20000
}

SECTIONS
{
    . = 0x78000000;

    .text : {
        KEEP(*(.text.entry))
        *(.text*)
    } > DRAM

    .rodata : { *(.rodata*) } > DRAM

    /* chainload_bl2_len must be the first symbol in .data so the
       injector can find and patch it at a fixed file offset. */
    .data : ALIGN(8) {
        chainload_bl2_len = .;
        QUAD(0)
        *(.data*)
    } > DRAM

    .bss : ALIGN(16) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > DRAM

    . = ALIGN(16);
    __stack_start = .;
    .stack : {
        . = . + 0x10000;  /* 64KB */
    } > DRAM
    __stack_end = .;

    ASSERT(. <= 0x78020000, "Payload exceeds the 0x20000 bl2_ext slot!")
}
