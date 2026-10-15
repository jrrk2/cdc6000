#!/usr/bin/env python3
"""rdextract.py diskdump outdir -- the live files of a ramdisk image (gendisk.py's layout) into outdir."""
import os, sys
img = open(sys.argv[1], 'rb').read(); out = sys.argv[2]; os.makedirs(out, exist_ok=True)
assert img[0:2] == b'CD', 'not a ramdisk image'
n = int.from_bytes(img[2:4], 'little')
live = {}
for e in range(n):
    b = 16 + 16 * e
    if img[b + 15]: continue
    name = img[b:b + 10].rstrip(b'\0').decode()
    blk, ln = int.from_bytes(img[b + 10:b + 12], 'little'), int.from_bytes(img[b + 12:b + 15], 'little')
    data = bytearray()
    while blk != 0xFFFF and len(data) < ln:
        data += img[blk * 1024:blk * 1024 + 1024]
        blk = int.from_bytes(img[1024 + 2 * blk:1026 + 2 * blk], 'little')
    live[name] = bytes(data[:ln])
for name, d in live.items():
    open(os.path.join(out, name), 'wb').write(d)
    print('%-10s %7d' % (name, len(d)))
