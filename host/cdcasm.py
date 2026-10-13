#!/usr/bin/env python3
"""Mini assembler for the CDC 6000 instructions the Pascal runtime needs.

usage: cdcasm.py in.asm out.img      (out.img: lines "<addr octal> <word octal 20 digits>")

Syntax (one statement per line, '#' starts a comment):
    NAME = expr            define a constant
    label:                 label (word aligned: the rest of the current word is padded with NO)
    .org expr              set the word address (octal numbers are written 77B)
    .word expr             one 60 bit data word (word aligned)
    mnemonic operands      e.g.  SA1 B1+3   SX6 X6+27   BX6 X6*X1   LX1 3   NZ X5,label   NE B2,B3,label   JP label
Numbers: 123 (decimal) or 77B (octal); expressions may use + - and labels.
"""
import re, sys

COND = {'ZR': 0, 'NZ': 1, 'PL': 2, 'NG': 3, 'IR': 4, 'OR': 5, 'DF': 6, 'ID': 7}
BCMP = {'EQ': 4, 'NE': 5, 'GE': 6, 'LT': 7}
BX = {'Xj': 0o10, 'Xj*Xk': 0o11, 'Xj+Xk': 0o12, 'Xj-Xk': 0o13, '-Xk': 0o14, '-Xk*Xj': 0o15, '-Xk+Xj': 0o16, '-Xk-Xj': 0o17}
FOP = {'FX+': 0o30, 'FX-': 0o31, 'DX+': 0o32, 'DX-': 0o33, 'RX+': 0o34, 'RX-': 0o35, 'IX+': 0o36, 'IX-': 0o37,
       'FX*': 0o40, 'RX*': 0o41, 'DX*': 0o42, 'FX/': 0o44, 'RX/': 0o45}
SBASE = {'SA': 0o50, 'SB': 0o60, 'SX': 0o70}
SFORM = {('A', '+', 'K'): 0, ('B', '+', 'K'): 1, ('X', '+', 'K'): 2, ('X', '+', 'B'): 3,
         ('A', '+', 'B'): 4, ('A', '-', 'B'): 5, ('B', '+', 'B'): 6, ('B', '-', 'B'): 7}


class Asm:
    def __init__(self):
        self.syms = {}
        self.items = []      # (kind, ...) in order
        self.errors = 0

    def err(self, ln, msg):
        print('line %d: %s' % (ln, msg), file=sys.stderr)
        self.errors += 1

    def num(self, text, ln):
        t = text.strip().upper()
        t = re.sub(r'\b([0-7]+)B\b', lambda m: str(int(m.group(1), 8)), t)
        t = re.sub(r'[A-Z_][A-Z_0-9]*', lambda m: str(self.syms[m.group(0)]) if m.group(0) in self.syms
                   else (_ for _ in ()).throw(KeyError(m.group(0))), t)
        return eval(t, {}, {})

    def parse(self, lines):
        for ln, raw in enumerate(lines, 1):
            s = raw.split('#')[0].strip()
            if not s:
                continue
            m = re.match(r'^([A-Za-z_][A-Za-z_0-9]*)\s*=\s*(.*)$', s)
            if m:
                self.items.append(('equ', ln, m.group(1).upper(), m.group(2)))
                continue
            m = re.match(r'^([A-Za-z_][A-Za-z_0-9]*):\s*(.*)$', s)
            if m:
                self.items.append(('label', ln, m.group(1).upper()))
                s = m.group(2).strip()
                if not s:
                    continue
            if s.startswith('.org'):
                self.items.append(('org', ln, s[4:]))
            elif s.startswith('.word'):
                self.items.append(('word', ln, s[5:]))
            elif s.startswith('.align'):
                self.items.append(('align', ln))
            else:
                self.items.append(('ins', ln, s))

    # ---- encode one instruction: returns list of parcels given resolved symbols ----
    def encode(self, ln, s):
        mn, _, op = s.partition(' ')
        mn = mn.upper(); op = op.replace(' ', '')
        u = op.upper()
        if mn == 'NO': return [0o46000]
        if mn == 'PS': return [0]
        def p15(fm, i, j, k): return [(fm << 9) | (i << 6) | (j << 3) | k]
        def p30(fm, i, j, K):
            K &= 0x3ffff
            return [(fm << 9) | (i << 6) | (j << 3) | (K >> 15), K & 0x7fff]
        def kv(t):
            v = self.num(t, ln)
            return v if v >= 0 else (~(-v)) & 0x3ffff
        m = re.match(r'^(BX)(\d)$', mn)
        if m:
            i = int(m.group(2))
            pat = re.sub(r'X(\d)', lambda q: 'X' + ('j' if False else q.group(1)), u)
            # normalise: map the operand's register numbers onto the j/k pattern
            regs = re.findall(r'X(\d)', u)
            sh = re.sub(r'X\d', 'X', u)
            # decide patterns by structure
            table = {'X': ('Xj', 0o10), 'X*X': ('Xj*Xk', 0o11), 'X+X': ('Xj+Xk', 0o12), 'X-X': ('Xj-Xk', 0o13),
                     '-X': ('-Xk', 0o14), '-X*X': ('-Xk*Xj', 0o15), '-X+X': ('-Xk+Xj', 0o16), '-X-X': ('-Xk-Xj', 0o17)}
            name, fm = table[sh]
            r = [int(x) for x in regs]
            if name == 'Xj': return p15(fm, i, r[0], 0)
            if name == '-Xk': return p15(fm, i, 0, r[0])
            if name in ('Xj*Xk', 'Xj+Xk', 'Xj-Xk'): return p15(fm, i, r[0], r[1])
            return p15(fm, i, r[1], r[0])            # -Xk*Xj : first register is k
        m = re.match(r'^(LX|AX)(\d)$', mn)
        if m:
            i = int(m.group(2)); base = 0o20 if m.group(1) == 'LX' else 0o21
            mm = re.match(r'^B(\d),X(\d)$', u)
            if mm: return p15(base + 2, i, int(mm.group(1)), int(mm.group(2)))
            n = self.num(op, ln)
            return p15(base, i, n >> 3, n & 7)
        m = re.match(r'^(NX|ZX|UX|PX)(\d)$', mn)
        if m:
            fm = {'NX': 0o24, 'ZX': 0o25, 'UX': 0o26, 'PX': 0o27}[m.group(1)]
            mm = re.match(r'^B(\d),X(\d)$', u)
            return p15(fm, int(m.group(2)), int(mm.group(1)), int(mm.group(2)))
        m = re.match(r'^(FX|DX|RX|IX)(\d)$', mn)
        if m:
            mm = re.match(r'^X(\d)([+\-*/])X(\d)$', u)
            fm = FOP[m.group(1) + mm.group(2)]
            return p15(fm, int(m.group(2)), int(mm.group(1)), int(mm.group(3)))
        m = re.match(r'^MX(\d)$', mn)
        if m:
            n = self.num(op, ln); return p15(0o43, int(m.group(1)), n >> 3, n & 7)
        m = re.match(r'^CX(\d)$', mn)
        if m:
            return p15(0o47, int(m.group(1)), 0, int(re.match(r'^X(\d)$', u).group(1)))
        m = re.match(r'^(SA|SB|SX)(\d)$', mn)
        if m:
            base = SBASE[m.group(1)]; i = int(m.group(2))
            mm = re.match(r'^([ABX])(\d)([+\-])(.*)$', op.upper())
            if not mm: raise ValueError('bad operand ' + op)
            rj, j, sg, rest = mm.group(1), int(mm.group(2)), mm.group(3), mm.group(4)
            mb = re.match(r'^B(\d)$', rest)
            if mb:
                off = SFORM[(rj, sg, 'B')]
                return p15(base + off, i, j, int(mb.group(1)))
            off = SFORM[(rj, '+', 'K')]
            return p30(base + off, i, j, kv(('-' if sg == '-' else '') + rest))
        if mn == 'JP':
            mm = re.match(r'^B(\d)\+(.*)$', u)
            if mm: return p30(2, int(mm.group(1)), 0, kv(mm.group(2)))
            return p30(2, 0, 0, kv(op))
        if mn == 'RJ': return p30(1, 0, 0, kv(op))
        if mn in COND:
            mm = re.match(r'^X(\d),(.*)$', u)
            return p30(3, COND[mn], int(mm.group(1)), kv(mm.group(2)))
        if mn in BCMP:
            mm = re.match(r'^B(\d),B(\d),(.*)$', u)
            return p30(BCMP[mn], int(mm.group(1)), int(mm.group(2)), kv(mm.group(3)))
        raise ValueError('unknown instruction ' + mn)

    def size(self, s):
        mn = s.split(' ')[0].upper()
        if mn in ('JP', 'RJ') or mn in COND or mn in BCMP: return 2
        if re.match(r'^(SA|SB|SX)\d$', mn):
            op = s.partition(' ')[2].replace(' ', '').upper()
            return 1 if re.match(r'^[ABX]\d[+\-]B\d$', op) else 2
        return 1

    def run(self):
        """two passes over items; returns {word address: 60 bit word}"""
        out = {}
        for pas in (1, 2):
            addr = 0; slot = 0; cur = 0
            out = {}
            def flush():
                nonlocal addr, slot, cur
                if slot:
                    while slot < 4:
                        cur = (cur << 15) | 0o46000; slot += 1
                    out[addr] = cur
                    addr += 1; slot = 0; cur = 0
            def emit(parcels):
                nonlocal addr, slot, cur
                for p in parcels:
                    cur = (cur << 15) | p; slot += 1
                    if slot == 4:
                        out[addr] = cur; addr += 1; slot = 0; cur = 0
            for it in self.items:
                k = it[0]
                try:
                    if k == 'equ':
                        self.syms[it[2]] = self.num(it[3], it[1]) if pas == 2 or True else 0
                    elif k == 'label':
                        flush(); self.syms[it[2]] = addr
                    elif k == 'org':
                        flush(); addr = self.num(it[2], it[1])
                    elif k == 'align':
                        flush()
                    elif k == 'word':
                        flush(); out[addr] = self.num(it[2], it[1]) & ((1 << 60) - 1); addr += 1
                    else:
                        s = it[2]
                        n = self.size(s)
                        if n == 2 and slot == 3:
                            emit([0o46000])
                        if pas == 1:
                            emit([0] * n)
                        else:
                            emit(self.encode(it[1], s))
                        if s.split(' ')[0].upper() == 'RJ':
                            flush()                  # the return jump goes to the next word
                except KeyError as e:
                    if pas == 2: self.err(it[1], 'undefined symbol %s' % e)
                    elif k == 'equ': pass
                except Exception as e:
                    self.err(it[1], '%s' % e)
            flush()
        return out


def main():
    a = Asm()
    a.parse(open(sys.argv[1]).read().split('\n'))
    out = a.run()
    if a.errors: sys.exit(1)
    with open(sys.argv[2], 'w') as f:
        for ad in sorted(out):
            f.write('%06o %020o\n' % (ad, out[ad]))
    # symbol table for debugging
    with open(sys.argv[2] + '.sym', 'w') as f:
        for k, v in sorted(a.syms.items(), key=lambda kv: kv[1]):
            f.write('%06o %s\n' % (v & 0xffffff, k))

if __name__ == '__main__':
    main()
