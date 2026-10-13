#!/usr/bin/env python3
"""randprog.py seed out.img [size] -- a random CDC 6000 program that always halts, for comparing cores.

The program is made of blocks: simple instructions of every unit kind (boolean, shift, normalise, long and floating add,
multiply, divide, address arithmetic, loads and stores), forward skips over a block (conditional branches of every kind),
counted loops (backward branches), and calls (RJ to subroutines placed after the main code, which end with JP to their entry).
Loads and stores only use B1-B3 (never written after the start) plus a small constant, so every memory reference falls in
the data area; B6 and B7 are loop counters; B4 and B5 and every X register take any value.  The program ends by jumping to
the HALT vector of the run time (host/runtime.img), so run it with that image: runtime.img prog.img.

Image lines are "address word" in octal, as cdcemu and the Verilated cores read them; words 1-3 are the boot block.
"""
import random
import sys

CODE = 0o2000
DATA = 0o4000
NO = 0o46000

COND = {'ZR': 0, 'NZ': 1, 'PL': 2, 'NG': 3, 'IR': 4, 'OR': 5, 'DF': 6, 'ID': 7}


class Gen:
    def __init__(self, seed, size):
        self.r = random.Random(seed)
        self.size = size
        self.items = []          # ('p', [parcels]) | ('long', f, i, j, K, label or None) | ('label', name) | ('word', value)
        self.nlab = 0
        self.subs = []

    # ---- encoding ----
    def p15(self, f, i, j, k):
        self.items.append(('p', [(f << 9) | (i << 6) | (j << 3) | k]))

    def p30(self, f, i, j, K, label=None):
        self.items.append(('long', f, i, j, K & 0x3ffff, label))

    def label(self):
        self.nlab += 1
        return 'L%d' % self.nlab

    def put_label(self, name):
        self.items.append(('label', name))

    # ---- instructions ----
    def reg_x(self):
        return self.r.randrange(8)

    def simple(self, calls_ok=True):
        r = self.r
        k = r.random()
        if k < 0.17:      # boolean
            f = r.choice([0o10, 0o11, 0o12, 0o13, 0o14, 0o15, 0o16, 0o17])
            self.p15(f, self.reg_x(), self.reg_x(), self.reg_x())
        elif k < 0.27:    # shifts
            if r.random() < 0.6:
                self.p15(r.choice([0o20, 0o21]), self.reg_x(), r.randrange(8), r.randrange(8))
            else:
                self.p15(r.choice([0o22, 0o23]), self.reg_x(), r.choice([0, 4, 5, 6, 7]), self.reg_x())
        elif k < 0.31:    # normalise, unpack, pack, count
            f = r.choice([0o24, 0o25, 0o26])
            self.p15(f, self.reg_x(), r.choice([0, 4, 5]), self.reg_x())
            if r.random() < 0.3:
                self.p15(0o27, self.reg_x(), r.choice([0, 4, 5]), self.reg_x())
            if r.random() < 0.3:
                self.p15(0o47, self.reg_x(), 0, self.reg_x())
        elif k < 0.40:    # long add
            self.p15(r.choice([0o36, 0o37]), self.reg_x(), self.reg_x(), self.reg_x())
        elif k < 0.48:    # floating add, multiply
            self.p15(r.choice([0o30, 0o31, 0o32, 0o33, 0o34, 0o35, 0o40, 0o41, 0o42]), self.reg_x(), self.reg_x(), self.reg_x())
        elif k < 0.49:    # divide
            self.p15(r.choice([0o44, 0o45]), self.reg_x(), self.reg_x(), self.reg_x())
        elif k < 0.51:    # mask
            self.p15(0o43, self.reg_x(), r.randrange(8), r.randrange(8))
        elif k < 0.52:
            self.p15(0o46, 0, 0, 0)                        # no-operation
        elif k < 0.60:    # SA0: any sources (A0 makes no memory reference)
            form = r.randrange(8)
            self.p30_or_15(0o50 + form, 0, r.randrange(8) if form != 1 else r.choice([0, 1, 2, 3, 4, 5, 6, 7]), r.randrange(8))
        elif k < 0.68:    # loads
            i = r.randrange(1, 6)
            self.memref(i)
        elif k < 0.74:    # stores
            self.memref(r.choice([6, 7]))
        elif k < 0.86:    # SB
            i = r.choice([4, 5, 0, 4, 5])
            self.p30_or_15(0o60 + r.randrange(8), i, r.randrange(8), r.randrange(8))
        else:             # SX
            self.p30_or_15(0o70 + r.randrange(8), self.reg_x(), r.randrange(8), r.randrange(8))

    def p30_or_15(self, f, i, j, k):
        """f is a SA/SB/SX family member: forms 0, 1, 2 take a constant K (30 bits), the others Bk (15 bits)."""
        form = f & 7
        if form <= 2:
            K = self.r.choice([self.r.randrange(0, 64), self.r.randrange(0, 0x3ffff), (~self.r.randrange(1, 30)) & 0x3ffff])      # the last is negative, ones complement
            self.p30(f, i, j, K)
        else:
            self.p15(f, i, j, k)

    def memref(self, i):
        """SAi Bj+K with j in 0..3: B0 + a data address, or B1-B3 (the bases) + a small offset"""
        j = self.r.choice([0, 1, 2, 3])
        if j == 0:
            K = DATA + self.r.randrange(0, 0x100)
        else:
            K = self.r.randrange(0, 0x40)
        self.p30(0o51, i, j, K)

    def cond_branch(self, label):
        r = self.r
        if r.random() < 0.6:
            self.p30(3, r.randrange(8), self.reg_x(), 0, label)
        else:
            self.p30(r.choice([4, 5, 6, 7]), r.choice([0, 4, 5]), r.choice([0, 4, 5]), 0, label)

    # ---- structure ----
    def block(self, depth, n, calls_ok):
        r = self.r
        count = 0
        while count < n:
            k = r.random()
            if k < 0.70 or depth > 2:
                self.simple()
                count += 1
            elif k < 0.85:                       # skip a block
                lab = self.label()
                self.cond_branch(lab)
                m = r.randrange(1, 6)
                self.block(depth + 1, m, False)
                self.put_label(lab)
                count += m + 1
            elif k < 0.95:                       # counted loop
                ctr = 7 if depth % 2 == 0 else 6
                if self.in_loop.get(ctr):
                    self.simple(); count += 1
                    continue
                top = self.label()
                self.in_loop[ctr] = True
                self.p30(0o61, ctr, 0, r.randrange(2, 5))      # SBc B0+count
                self.put_label(top)
                m = r.randrange(2, 7)
                self.block(depth + 1, m, False)
                self.p30(0o61, ctr, ctr, 0o777776)             # SBc Bc+(-1), ones complement
                self.p30(5, ctr, 0, 0, top)                    # NE Bc,B0,top
                self.in_loop[ctr] = False
                count += m + 3
            elif calls_ok and self.nsubs < 4:
                self.nsubs += 1
                name = 'S%d' % self.nsubs
                self.subs.append(name)
                self.p30(1, 0, 0, 0, name)                     # RJ name
                self.put_label(self.label())                   # the return is to the next word
                count += 1
            else:
                self.simple()
                count += 1

    def build(self):
        r = self.r
        self.in_loop = {}
        self.nsubs = 0
        # base registers and a start state
        self.p30(0o61, 1, 0, DATA)             # B1 = data base
        self.p30(0o61, 2, 0, DATA + 0x40)
        self.p30(0o61, 3, 0, DATA + 0x80)
        for i in (4, 5):
            self.p30(0o61, i, 0, r.randrange(0, 0x3ffff))
        for i in range(1, 6):
            self.p30(0o51, i, 0, DATA + r.randrange(0, 0x100))   # SAi: load X1-X5
        self.p30(0o70, 0, 0, r.randrange(0, 0x3ffff))
        self.p30(0o70, 6, 0, r.randrange(0, 0x3ffff))
        self.p30(0o70, 7, 0, r.randrange(0, 0x3ffff))
        self.block(0, self.size, True)
        self.p30(2, 0, 0, 0o100)               # JP HALT vector
        # subroutines: a free word for the return jump, a body, JP back to the word
        done = set()
        while True:
            todo = [s for s in self.subs if s not in done]
            if not todo:
                break
            for s in todo:
                done.add(s)
                self.put_label(s)
                self.items.append(('word', 0))                 # RJ stores the return jump here
                self.block(2, r.randrange(2, 8), False)
                self.p30(2, 0, 0, 0, s)                        # JP s

    def assemble(self):
        # layout: parcels in order; a 30 bit instruction never starts in the last parcel of a word; labels start a word
        pos = 0               # parcel position
        out = []              # (parcel position, item)
        labels = {}
        for it in self.items:
            if it[0] == 'label':
                while pos % 4:
                    out.append((pos, ('p', [NO]))); pos += 1
                labels[it[1]] = CODE + pos // 4
            elif it[0] == 'word':
                while pos % 4:
                    out.append((pos, ('p', [NO]))); pos += 1
                out.append((pos, it)); pos += 4
            elif it[0] == 'p':
                out.append((pos, it)); pos += 1
            else:
                if pos % 4 == 3:
                    out.append((pos, ('p', [NO]))); pos += 1
                out.append((pos, it)); pos += 2
        while pos % 4:
            out.append((pos, ('p', [NO]))); pos += 1
        words = {}
        for pp, it in out:
            if it[0] == 'p':
                parcels = it[1]
            elif it[0] == 'word':
                words[CODE + pp // 4] = it[1]
                continue
            else:
                _, f, i, j, K, lab = it
                if lab is not None:
                    K = labels[lab]
                parcels = [(f << 9) | (i << 6) | (j << 3) | (K >> 15), K & 0x7fff]
            for q, pc in enumerate(parcels):
                w = CODE + (pp + q) // 4
                s = (pp + q) % 4
                words[w] = words.get(w, 0) | (pc << (45 - 15 * s))
        return words


def main():
    seed = int(sys.argv[1]); out = sys.argv[2]
    size = int(sys.argv[3]) if len(sys.argv) > 3 else 60
    g = Gen(seed, size)
    g.build()
    words = g.assemble()
    r = random.Random(seed * 7919 + 1)
    for a in range(DATA, DATA + 0x300):
        k = r.random()
        if k < 0.3:
            words[a] = r.getrandbits(60)
        elif k < 0.5:
            words[a] = r.randrange(0, 1 << 18)
        elif k < 0.65:
            words[a] = ((1 - r.randrange(2)) * 0) | (r.randrange(0o1770, 0o2020) << 48) | r.getrandbits(48)     # a float
            if r.random() < 0.3:
                words[a] ^= (1 << 60) - 1                                                                      # negated
        elif k < 0.75:
            words[a] = 0
        else:
            words[a] = r.getrandbits(60) & r.getrandbits(60)
    words[1] = CODE
    words[2] = 0o10000
    words[3] = 0o20000
    with open(out, 'w') as f:
        for a in sorted(words):
            f.write('%06o %020o\n' % (a, words[a] & ((1 << 60) - 1)))


main()
