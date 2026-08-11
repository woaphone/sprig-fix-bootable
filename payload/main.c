/* Modified by 秋逸(逸) <2898684403@qq.com> */
#include <bldr.h>
#include <chainload.h>
#include <debug.h>
#include <mmio.h>
#include <patches.h>
#include <target.h>

int main(void) {
    printf("\n");
    printf("           .--._.--.          \n");
    printf("          ( O     O )         \n");
    printf("          /   . .   \\         \n");
    printf("         .`._______.'.        \n");
    printf("        /(           )\\                              _       \n");
    printf("      _/  \\  \\   /  /  \\_           ___ _ __  _ __(_) __ _ \n");
    printf("   .~   `  \\  \\ /  /  '   ~.       / __| '_ \\| '__| |/ _` |\n");
    printf("  {    -.   \\  V  /   .-    }      \\__ \\ |_) | |  | | (_| |\n");
    printf("_ _`.    \\  |  |  |  /    .'_ _    |___/ .__/|_|  |_|\\__, |\n");
    printf(">_       _} |  |  | {_       _<         |_|           |___/ \n");
    printf(" /. - ~ ,_-'  .^.  `-_, ~ - .\\      \n");
    printf("         '-'|/   \\|`-`              \n\n");

    if (patch_apply_all() != 0) {
        printf("Patch verification failed, refusing to continue.\n");
        while (1);
    }

    /* OPPO may have latched usbEnum off in an earlier boot path, which
       makes usb_connect() fail inside the handshake. Clear it. */
    writeb(0, OPPO_USB_ENUM_LOCK);
    flush_dcache_range(OPPO_USB_ENUM_LOCK, 64);

    printf("About to handshake...\n\n");

    int r = bldr_handshake();
    printf("handshake returned %d\n", r);

    /*
     * The handshake returns when no successful DA session is in
     * progress (a successful DA download jumps away inside the
     * handshake and never returns). So unconditionally restore the
     * original bl2_ext and continue the normal boot chain - this
     * matches the behavior of the working reference payload.
     */
    chainload_to_bl2();

    while (1);
}
