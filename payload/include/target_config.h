/* Created by 秋逸(逸) <2898684403@qq.com> */
#pragma once

/*
 * Per-target configuration.
 *
 * The preloader addresses below are firmware-specific: every build of the
 * preloader can move these functions. Generate this file for your target
 * by reverse-engineering your firmware's preloader (see README_en.md) and
 * place it at payload/include/target_config.h.
 *
 * The values below are an example (MediaTek MT6991 reference firmware).
 */

#define TARGET_LK_IMAGE        "bin/lk.img"
#define TARGET_NAME            "example"

#define BLDR_HANDSHAKE_ADDR    0x020656F8
#define BLDR_CALLBACK_ADDR     0x0206DCD4

#define PATCH_SUSBDL_ADDR      0x020AB714
#define PATCH_SUSBDL_EXPECT0   0xD503233F
#define PATCH_SUSBDL_EXPECT1   0xA9BF7BFD

#define PATCH_SBC_ADDR         0x020B4448
#define PATCH_DAA_ADDR         0x020B4464
#define PATCH_SLA_ADDR         0x020B4480
#define PATCH_GETTER_EXPECT0   0xD503233F
#define PATCH_GETTER_EXPECT1   0x52800C08

#define OPPO_USB_ENUM_LOCK     0x02128C7C

#define PATCH_TIMEOUT_ADDR      0x0206575C
#define PATCH_TIMEOUT_EXPECT0   0x5283E817
#define PATCH_TIMEOUT_EXPECT1   0xB0000302
#define PATCH_TIMEOUT_NEW0      0x5283FA17

#define TRAMPOLINE_ADDR         0xB8201C00UL
