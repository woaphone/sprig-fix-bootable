/* Modified by 秋逸(逸) <2898684403@qq.com> */
#include <debug.h>
#include <mmio.h>
#include <patches.h>
#include <target.h>

static const patch_t patches[] = {
    PATCH_RET_CHECKED(PATCH_SUSBDL_ADDR, PATCH_SUSBDL_EXPECT0, PATCH_SUSBDL_EXPECT1, 0, "Bypass SUSBDL/DA1 verification"),
    PATCH_RET_CHECKED(PATCH_SBC_ADDR, PATCH_GETTER_EXPECT0, PATCH_GETTER_EXPECT1, 0, "Disable SBC"),
    PATCH_RET_CHECKED(PATCH_DAA_ADDR, PATCH_GETTER_EXPECT0, PATCH_GETTER_EXPECT1, 0, "Disable DAA"),
    PATCH_RET_CHECKED(PATCH_SLA_ADDR, PATCH_GETTER_EXPECT0, PATCH_GETTER_EXPECT1, 0, "Disable SLA"),
};

int patch_apply_all(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(patches); i++) {
        const patch_t *p = &patches[i];
        uint32_t w0 = readl(p->addr);
        uint32_t w1 = readl(p->addr + 4);

        if (p->expect[0] != 0) {
            if (w0 != p->expect[0] || w1 != p->expect[1]) {
                printf("PATCH FAIL %-30s @ 0x%08x: got 0x%08x 0x%08x, want 0x%08x 0x%08x\n",
                       p->name, p->addr, w0, w1, p->expect[0], p->expect[1]);
                return -1;
            }
        }

        writel(p->insn[0], p->addr);

        if (p->insn[1] != 0) {
            writel(p->insn[1], p->addr + 4);
            printf("%-30s 0x%08x: 0x%08x 0x%08x -> 0x%08x 0x%08x\n",
                   p->name, p->addr, w0, w1, p->insn[0], p->insn[1]);
        } else {
            printf("%-30s 0x%08x: 0x%08x -> 0x%08x\n",
                   p->name, p->addr, w0, p->insn[0]);
        }

        flush_dcache_range(p->addr, 64);
    }

    invalidate_icache();
    return 0;
}
