/* Modified by 秋逸(逸) <2898684403@qq.com> */
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

int bldr_handshake(void);
