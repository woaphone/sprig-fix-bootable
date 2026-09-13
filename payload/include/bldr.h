#pragma once

#include <stdint.h>
#include <stddef.h>
#include <target.h>

struct bldr_command_handler {
    void *priv;
    uint32_t attr;
    void *cb;
};

#define BLDR_HANDSHAKE_FUNC  BLDR_HANDSHAKE_ADDR
#define BLDR_CALLBACK_FUNC   BLDR_CALLBACK_ADDR
#define BLDR_LOG_FUNC        PL_LOG_PRINTF_ADDR
#define PL_UDELAY_FUNC       PL_UDELAY_ADDR

#define BLDR_ERR_PREPARE     (-0x5301)
#define BLDR_ERR_RESTORE     (-0x5302)

int bldr_handshake(void);
