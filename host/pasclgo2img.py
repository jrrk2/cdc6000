#!/usr/bin/env python3
"""Turn the compiler's output (pasclgo.out + parmlist.out) into a loadable image for cdcemu / the RTL.

usage: pasclgo2img.py pasclgo.out parmlist.out program.img [--compiler]

Boot block (words 1..3, below the I/O words): 1 = entry point, 2 = B5 (frame base), 3 = B4 (stack limit).
--compiler stores 2048 into PARMLIST.ICO (word ICO+1) so that the image, when it is the Pascal compiler, produces
programs with ICO 2048.  The jump table (the 50 extra words at the end of the stream) is copied to LPJMPTAB.
"""
import sys
pf, parm, out = sys.argv[1:4]
comp = '--compiler' in sys.argv
P = {}
for l in open(parm):
    k, v = l.split(); P[k] = int(v)
words = [int(l, 8) for l in open(pf) if l.strip()]
img = {}
n = P['LIMCODE'] - P['LOADPT']
for i, w in enumerate(words[:n]): img[P['LOADPT'] + i] = w
for i, w in enumerate(words[n:n + 50]): img[P['LPJMPTAB'] + i] = w
if comp: img[P['ICO'] + 1] = int(next((a.split('=')[1] for a in sys.argv if a.startswith('--ico=')), '2048'))
if '--noboot' not in sys.argv:
    img[1] = P['ENTRYPT']; img[2] = P['LIMCODE'] + 8; img[3] = int(next((a.split('=')[1] for a in sys.argv if a.startswith('--b4=')), '0x1ff00'), 0)
with open(out, 'w') as f:
    for a in sorted(img): f.write('%06o %020o\n' % (a, img[a]))
