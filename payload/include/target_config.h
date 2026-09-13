#pragma once

/*
 * Per-target configuration for Xiaomi "rothko" (MT6989), engineering
 * (debug factory) preloader only -- the retail preloader does not contain
 * the usbdl/DA1 download path this payload drives.
 *
 * Preloader load address: 0x02000F00 (data at 0x02001000).
 * bl2_ext load address:   0x78000000 (res_mem_info "system_bl2-ext",
 *                                    32MB window).
 *
 * All patch sites verified against preloader_rothko.bin
 * (FACTORY-ROTHKO-0820 debug build, 941444 bytes, sha check your own copy).
 * Each site keeps its original instruction words as expect values so a
 * mismatched firmware fails safely instead of blind-patching.
 */

#define TARGET_LK_IMAGE        "bin/lk.img"
#define TARGET_NAME            "rothko_mt6989"

/* bldr_handshake: USB handshake + DA1 download loop (prints "READY",
   runs the usbdl command processor at 0x02008904).
   Called from the preloader boot flow at 0x0205F31C. */
#define BLDR_HANDSHAKE_ADDR    0x020585E0

/* Command handler the preloader's own boot flow installs in
   struct bldr_command_handler {priv, attr, cb} (cb at offset 0x10,
   stored at 0x0205F1D8). */
#define BLDR_CALLBACK_ADDR     0x0205F604

/* PL functions/data verified against this same engineering image. */
#define PL_LOG_PRINTF_ADDR     0x02078738UL
#define PL_CHG_TYPE_CACHE      0x020EC028UL
#define PL_CHG_TYPE_VALID      0x020EC02CUL

/* Optional SRAM candidate: only the low permission fields may be changed. */
#define PL_SRAM_SEC_CTRL       0x1001C010UL
#define PL_SRAM_SEC_CTRL2      0x1001C018UL
#define PL_SRAM_SEC_ADDR       0x1001C050UL
#define PL_SRAM_SEC_ADDR1      0x1001C054UL
#define PL_SRAM_SEC_ADDR2      0x1001C058UL
#define PL_SRAM_PERM_MASK      0x00000FFFU
#define PL_SRAM_EARLY_CTRL     0x80000000U
#define PL_SRAM_LATE_CTRL      0x80000B69U
#define PL_SRAM_LATE_CTRL2     0x00000B6DU
#define PL_SRAM_LATE_ADDR      0x90012000U
#define PL_SRAM_LATE_BOUND     0x00012000U

/* usbdl_vfy_da gate: returns 1 when the DA image must be verified.
   Returning 0 hits the "usbdl_vfy_da:disabled" branch and the DA is
   accepted without signature/hash verification. */
#define PATCH_SUSBDL_ADDR      0x02090218
#define PATCH_SUSBDL_EXPECT0   0xA9BF7BFD    /* stp x29, x30, [sp, #-16]! */
#define PATCH_SUSBDL_EXPECT1   0x910003FD    /* mov x29, sp */

/* Security-flag getters (read bits 1/2/3 of the flags word at
   0x111F1060). Bit1 = SLA, bit2 = DAA, bit3 = SBC (order per the
   handshake response builder at 0x02008E18). Patched to mov w0,#0; ret
   so the DA session reports all checks disabled. */
#define PATCH_SLA_ADDR         0x02099A50
#define PATCH_DAA_ADDR         0x02099A64
#define PATCH_SBC_ADDR         0x02099A78
#define PATCH_GETTER_EXPECT0   0x52800C08    /* mov w8, #0x60 */
#define PATCH_GETTER_EXPECT1   0x72A23E28    /* movk w8, #0x11f1, lsl #16 */

/* Xiaomi rothko has no OPPO-style usbEnum latch; the write in main.c
   is compiled out (OPPO_USB_ENUM_LOCK undefined). */

/* Tool window: the UART/USB handshake already waits 2500ms + 8000ms
   (w28=0x9C4 / w23=0x1F40 at 0x02058644), so no timeout patch is
   required on this firmware. */

/* Safe window for the chainload trampoline: inside the 32MB
   system_bl2-ext reservation, past the composite image
   (0x78020000 + 0x130E18 = 0x78150E18). */
#define TRAMPOLINE_ADDR         0x78160000UL
