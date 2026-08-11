# Modified by 秋逸(逸) <2898684403@qq.com>
#!/usr/bin/env python3
"""Chainload injector for the sprig MT6991 payload.

Replaces the bl2_ext sub-partition in an LK image with a composite:
  [payload.bin padded to 0x20000][original bl2_ext]

The original bl2_ext is copied back over the payload by the chainload
trampoline when the handshake times out, restoring the normal boot.
"""
import struct
import sys
from liblk.image import LkImage

PAYLOAD_SLOT = 0x20000
BL2_LOAD = 0xB8000000


def main():
    if len(sys.argv) != 4:
        print("Usage: %s <lk_image> <payload.bin> <output>" % sys.argv[0])
        return 1

    lk_path = sys.argv[1]
    pl_path = sys.argv[2]
    out_path = sys.argv[3]

    print("\nLoading: %s" % lk_path)
    img = LkImage(lk_path)

    if 'bl2_ext' not in img.partitions:
        print("Error: No bl2_ext partition")
        return 1

    with open(pl_path, 'rb') as f:
        payload = f.read()

    bl2 = img.partitions['bl2_ext'].data
    bl2_len = len(bl2)

    if len(payload) > PAYLOAD_SLOT:
        print("Error: payload (%d) exceeds the 0x%x slot" % (len(payload), PAYLOAD_SLOT))
        return 1

    if bl2_len % 8 != 0:
        print("Error: original bl2_ext length 0x%x is not 8-aligned" % bl2_len)
        return 1

    print("Original bl2_ext size: 0x%x bytes" % bl2_len)
    print("New payload size: %d bytes" % len(payload))

    # Patch the bl2 length into the payload (chainload_bl2_len, first
    # symbol in .data). Read the file offset from the build artifacts.
    offset_file = 'payload/build/bl2_len_offset.txt'
    try:
        with open(offset_file, 'r') as f:
            len_off = int(f.read().strip())
    except (FileNotFoundError, ValueError):
        print("Error: cannot read %s" % offset_file)
        return 1

    if len_off + 8 > len(payload):
        print("Error: bl2_len offset 0x%x out of payload bounds" % len_off)
        return 1

    payload = bytearray(payload)
    payload[len_off:len_off + 8] = struct.pack('<Q', bl2_len)
    print("Patched bl2_len (0x%x) at payload offset 0x%x" % (bl2_len, len_off))

    # Build the composite image.
    composite = bytes(payload) + b'\x00' * (PAYLOAD_SLOT - len(payload))
    composite += bl2
    print("Composite bl2_ext: 0x%x + 0x%x = 0x%x bytes" %
          (PAYLOAD_SLOT, bl2_len, len(composite)))

    part = img.partitions['bl2_ext']
    part.data = composite

    img._rebuild_contents()

    print("Saving: %s" % out_path)
    img.save(out_path)
    print("Done!\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
