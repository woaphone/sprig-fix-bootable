/* MT6989 engineering-preloader session preparation and rollback. */
#include <bldr.h>
#include <mmio.h>

#ifndef SPRIG_RESTORE_SRAM
#define SPRIG_RESTORE_SRAM 0
#endif

typedef int (*handshake_fn_t)(struct bldr_command_handler *);
typedef int (*pl_log_fn_t)(const char *, ...);

#define pl_log(...) ((pl_log_fn_t)BLDR_LOG_FUNC)(__VA_ARGS__)

struct session_state {
    uint32_t charger_type;
    uint32_t charger_valid;
#if SPRIG_RESTORE_SRAM
    uint32_t sram_ctrl;
    uint32_t sram_ctrl2;
    int sram_touched;
#endif
};

static void publish_charger_cache(void)
{
    flush_dcache_range(PL_CHG_TYPE_CACHE & ~0x3FUL, 64);
    dsb(sy);
}

#if SPRIG_RESTORE_SRAM
static int prepare_sram(struct session_state *saved)
{
    uint32_t addr = readl(PL_SRAM_SEC_ADDR);
    uint32_t addr1 = readl(PL_SRAM_SEC_ADDR1);
    uint32_t addr2 = readl(PL_SRAM_SEC_ADDR2);
    int known_bounds;
    int known_permissions;

    saved->sram_ctrl = readl(PL_SRAM_SEC_CTRL);
    saved->sram_ctrl2 = readl(PL_SRAM_SEC_CTRL2);
    known_bounds =
        (addr == PL_SRAM_LATE_ADDR && addr1 == PL_SRAM_LATE_BOUND &&
         addr2 == PL_SRAM_LATE_BOUND) ||
        (addr == 0 && addr1 == 0 && addr2 == 0);
    known_permissions =
        (saved->sram_ctrl == PL_SRAM_LATE_CTRL &&
         saved->sram_ctrl2 == PL_SRAM_LATE_CTRL2) ||
        (saved->sram_ctrl == PL_SRAM_EARLY_CTRL && saved->sram_ctrl2 == 0);

    pl_log("[sprig-v5] SRAM ctrl=%08x/%08x bounds=%08x/%08x/%08x\n",
           saved->sram_ctrl, saved->sram_ctrl2, addr, addr1, addr2);
    if (!known_bounds || !known_permissions) {
        pl_log("[sprig-v5] unrecognized SRAM policy; download cancelled\n");
        return -1;
    }

    /* Preserve boundary/enable bits. Only test the audited permission fields. */
    saved->sram_touched = 1;
    writel(saved->sram_ctrl & ~PL_SRAM_PERM_MASK, PL_SRAM_SEC_CTRL);
    writel(saved->sram_ctrl2 & ~PL_SRAM_PERM_MASK, PL_SRAM_SEC_CTRL2);
    dsb(sy);
    isb();
    if (readl(PL_SRAM_SEC_CTRL) != (saved->sram_ctrl & ~PL_SRAM_PERM_MASK) ||
        readl(PL_SRAM_SEC_CTRL2) != (saved->sram_ctrl2 & ~PL_SRAM_PERM_MASK)) {
        pl_log("[sprig-v5] SRAM write did not read back; download cancelled\n");
        return -1;
    }
    pl_log("[sprig-v5] SRAM permission fields cleared; readback verified\n");
    return 0;
}
#endif

static int restore_session(const struct session_state *saved)
{
    int failed = 0;

#if SPRIG_RESTORE_SRAM
    if (saved->sram_touched) {
        writel(saved->sram_ctrl2, PL_SRAM_SEC_CTRL2);
        writel(saved->sram_ctrl, PL_SRAM_SEC_CTRL);
        dsb(sy);
        isb();
        if (readl(PL_SRAM_SEC_CTRL) != saved->sram_ctrl ||
            readl(PL_SRAM_SEC_CTRL2) != saved->sram_ctrl2)
            failed = 1;
    }
#endif
    writel(saved->charger_type, PL_CHG_TYPE_CACHE);
    writel(saved->charger_valid, PL_CHG_TYPE_VALID);
    publish_charger_cache();
    if (readl(PL_CHG_TYPE_CACHE) != saved->charger_type ||
        readl(PL_CHG_TYPE_VALID) != saved->charger_valid)
        failed = 1;

    if (failed)
        pl_log("[sprig-v5] ROLLBACK FAILED; refusing normal boot\n");
    return failed ? BLDR_ERR_RESTORE : 0;
}

int bldr_handshake(void)
{
    struct bldr_command_handler handler = {
        .priv = NULL,
        .attr = 0,
        .cb = (void *)BLDR_CALLBACK_FUNC
    };

    struct session_state saved = {0};
    handshake_fn_t handshake_fn = (handshake_fn_t)BLDR_HANDSHAKE_FUNC;
    int result = BLDR_ERR_PREPARE;

    saved.charger_type = readl(PL_CHG_TYPE_CACHE);
    saved.charger_valid = readl(PL_CHG_TYPE_VALID);
    pl_log("[sprig-v5] begin SRAM_candidate=%u\n", SPRIG_RESTORE_SRAM);
    writel(1, PL_CHG_TYPE_CACHE);
    writel(1, PL_CHG_TYPE_VALID);
    publish_charger_cache();
    if (readl(PL_CHG_TYPE_CACHE) != 1 || readl(PL_CHG_TYPE_VALID) != 1) {
        pl_log("[sprig-v5] charger cache write failed; download cancelled\n");
        goto restore;
    }
#if SPRIG_RESTORE_SRAM
    if (prepare_sram(&saved) != 0)
        goto restore;
#endif

    /* A successful DA jump does not return. All returning paths roll back. */
    result = handshake_fn(&handler);
restore:
    if (restore_session(&saved) != 0)
        return BLDR_ERR_RESTORE;
    pl_log("[sprig-v5] session restored; chainload allowed (result=%d)\n", result);
    return result;
}
