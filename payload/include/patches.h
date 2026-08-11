/* Modified by 秋逸(逸) <2898684403@qq.com> */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ARM64_NOP           0xd503201f
#define ARM64_RET           0xd65f03c0
#define ARM64_MOV_W0_IMM(n) (0x52800000 | ((n) << 5))

typedef struct {
    uint32_t addr;
    uint32_t expect[2];
    uint32_t insn[2];
    const char *name;
} patch_t;

#define PATCH(a, v, n) \
    { .addr = (a), .expect = {0, 0}, .insn = {(v), 0}, .name = (n) }

#define PATCH2(a, v1, v2, n) \
    { .addr = (a), .expect = {0, 0}, .insn = {(v1), (v2)}, .name = (n) }

#define PATCH_CHECKED(a, e0, e1, v1, v2, n) \
    { .addr = (a), .expect = {(e0), (e1)}, .insn = {(v1), (v2)}, .name = (n) }

#define PATCH_NOP(a, n) \
    PATCH((a), ARM64_NOP, (n))

#define PATCH_RET(a, r, n) \
    PATCH2((a), ARM64_MOV_W0_IMM(r), ARM64_RET, (n))

#define PATCH_RET_CHECKED(a, e0, e1, r, n) \
    PATCH_CHECKED((a), (e0), (e1), ARM64_MOV_W0_IMM(r), ARM64_RET, (n))

int patch_apply_all(void);
