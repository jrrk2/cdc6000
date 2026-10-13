#!/usr/bin/env python3
"""Generate a machine-code program that applies every arithmetic/shift/float instruction to many operand pairs
and stores the results; run it on cdcemu and on the RTL core and compare the memory dumps.
usage: fptest.py out.img [ncases]"""
import random, sys
random.seed(6600)
out, ncases = sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 60
M = (1 << 60) - 1
def fl(neg, e, c):          # a float word: e biased exponent field, c coefficient
    w = ((e & 0xfff) << 48) | (c & ((1 << 48) - 1))
    return (~w & M) if neg else w
def rnd_operand():
    r = random.random()
    if r < 0.08: return 0
    if r < 0.12: return M                                            # -0
    if r < 0.30:                                                     # integers, positive and negative
        v = random.choice([random.randrange(0, 1 << 16), random.randrange(0, 1 << 40), random.randrange(0, 1 << 47)])
        return (~v & M) if random.random() < 0.4 else v
    if r < 0.45: return random.randrange(0, 1 << 60)                 # any bits
    c = random.randrange(1 << 47, 1 << 48) if random.random() < 0.8 else random.randrange(1, 1 << 48)
    e = 1024 + random.randrange(-60, 60) if random.random() < 0.7 else random.randrange(0, 4096)
    return fl(random.random() < 0.4, e, c)
ops = ['BX6 X1', 'BX6 X1*X2', 'BX6 X1+X2', 'BX6 X1-X2', 'BX6 -X2', 'BX6 -X2*X1', 'BX6 -X2+X1', 'BX6 -X2-X1',
       'FX6 X1+X2', 'FX6 X1-X2', 'DX6 X1+X2', 'DX6 X1-X2', 'RX6 X1+X2', 'RX6 X1-X2', 'IX6 X1+X2', 'IX6 X1-X2',
       'FX6 X1*X2', 'RX6 X1*X2', 'DX6 X1*X2', 'FX6 X1/X2', 'RX6 X1/X2', 'CX6 X2',
       'LX6 B3,X1', 'AX6 B3,X1', 'NX6 B2,X1', 'ZX6 B2,X1', 'UX6 B2,X1', 'PX6 B3,X1']
lines = ['IO_HALT = 20', '.org 1', '.word 1000B', '.word 20000B', '.word 177400B']   # boot block: entry, B5, B4
lines.append('.org 1000B')
data = []
cases = [(rnd_operand(), rnd_operand(), random.choice([0, 1, 5, 17, 47, 59, 60, 61, 63, 0o777777, 0o777776, 0o777700, 0o777700 + 17, 100, 3, 0o20000]))
         for _ in range(ncases)]
for (a, b, s) in cases:
    data += [a, b, s]
START = 'START:'
lines.append(START)
res = 0
shifts = []
for n in range(ncases):
    lines.append('  SA1 B0+TAB+%d' % (3 * n))
    lines.append('  SA2 B0+TAB+%d' % (3 * n + 1))
    lines.append('  SA3 B0+TAB+%d' % (3 * n + 2))
    lines.append('  SB3 X3+0')
    for o in ops:
        lines.append('  SB2 B0+0')
        lines.append('  %s' % o)
        lines.append('  SA6 B0+RES+%d' % res); res += 1
        lines.append('  SX5 B2+0')
        lines.append('  SA5 B0+RES+%d' % res); res += 1       # B2 after NX/UX (store X5 -- need SA5 as store: use SA7)
lines.append('  SX6 B0+1')
lines.append('  SA6 B0+IO_HALT')
# fix store of X5: use X7 so that SA7 stores
txt = '\n'.join(lines).replace('  SX5 B2+0\n  SA5 B0+RES', '  SX7 B2+0\n  SA7 B0+RES')
# data tables placed after the code; compute code size generously
txt += '\n.org 200000B\nTAB:\n' + '\n'.join('.word %d' % d for d in data)
txt += '\n.org 300000B\nRES:\n'
open(out + '.asm', 'w').write(txt + '\n')
