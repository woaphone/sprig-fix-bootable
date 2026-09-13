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

#ifdef OPPO_USB_ENUM_LOCK
    /* OPPO may have latched usbEnum off in an earlier boot path, which
       makes usb_connect() fail inside the handshake. Clear it. Not
       needed on Xiaomi rothko. */
    writeb(0, OPPO_USB_ENUM_LOCK);
    flush_dcache_range(OPPO_USB_ENUM_LOCK, 64);
#endif

    printf("About to handshake...\n\n");

    int r = bldr_handshake();
    if (r == BLDR_ERR_RESTORE) {
        while (1)
            __asm__ volatile("wfe");
    }
    printf("handshake returned %d\n", r);

    /*
     * The session has restored any temporary cache/permission changes.
     * The stock bl2_ext owns its normal UFS initialization.
     */
    chainload_to_bl2();

    while (1);
}
