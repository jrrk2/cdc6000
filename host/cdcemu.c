/* CDC 6000 CPU emulator: the reference model for rtl/cdc6000_core.sv.
 *
 *   cdcemu runtime.img program.img [options]
 *
 * Pure machine: 60 bit words, X/A/B registers, and five memory-mapped I/O words (see mkrt.py):
 *   020B IO_RX (load)   021B IO_TX   022B IO_FILE   023B IO_FWORD   024B IO_HALT (stores)
 * The runtime (GETC, PUTC, WRT*, ...) is CDC machine code in runtime.img, not C.
 * program.img: "addr word" lines (octal), as written by pasclgo2img.py.
 * Options: -i file  input text (default stdin)   -o file  character output (default stdout)
 *          -d dir   directory for binary files (named lowercase(LFN).out)   -n N  stop after N instructions
 *          -w addr  report stores to addr   -j lo hi  trace jumps in that instruction range
 *          -x addr  dump memory words from addr (octal) to stderr at halt (with -xn count)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

typedef uint64_t u64;
typedef unsigned __int128 u128;
#define M60 ((1ULL << 60) - 1)
#define M48 ((1ULL << 48) - 1)
#define M18 0x3ffffU
#define MEMW (1 << 18)
#define IO_RX 020
#define IO_TX 021
#define IO_FILE 022
#define IO_FWORD 023
#define IO_HALT 024

static u64 *mem;
static u64 X[8];
static unsigned A[8], B[8];
static unsigned P;
static long long ninstr, maxinstr, jlo, jhi;
static unsigned watch; static int tracei; static const char *dumpf;

/* ---------- 18 bit / 60 bit ones-complement arithmetic ---------- */
static unsigned add18(unsigned a, unsigned b)
{
    unsigned s = a + b;
    if (s > M18) s = (s & M18) + 1;
    s &= M18;
    if (s == M18 && !(a == M18 && b == M18)) s = 0;     /* x + -x = +0 */
    return s;
}
static int sb18(unsigned a) { return (a & 0x20000) ? -(int)(~a & M18) : (int)a; }
static unsigned neg18(unsigned a) { return ~a & M18; }
static u64 sx18(unsigned a) { return (a & 0x20000) ? ((u64)a | (M60 & ~(u64)M18)) : (u64)a; }
static u64 add60(u64 a, u64 b)
{
    u64 s = a + b;
    if (s > M60) s = (s & M60) + 1;
    s &= M60;
    if (s == M60 && !(a == M60 && b == M60)) s = 0;     /* x + -x = +0 */
    return s;
}

/* ---------- floating point ---------- */
typedef struct { int neg; int e; u64 c; } fl;     /* value = c * 2^e (e unbiased: field-1024) */
static fl funpack(u64 w)
{
    fl f; f.neg = (w >> 59) & 1;
    if (f.neg) w = ~w & M60;
    f.e = (int)((w >> 48) & 0xfff) - 1024;
    f.c = w & M48;
    return f;
}
static u64 fpack(int neg, int e, u64 c)
{
    u64 w = ((u64)((e + 1024) & 0xfff) << 48) | (c & M48);
    return neg ? (~w & M60) : w;
}
/* normalise/round a signed 128 bit magnitude*2^e into a word; mode: 0 truncate, 1 round */
static u64 fnorm(int neg, u128 m, int e, int round)
{
    int p, sh;
    if (m == 0) return 0;
    p = 127; while (!((m >> p) & 1)) p--;
    sh = p - 47;
    if (sh > 0) {
        if (round) m += (u128)1 << (sh - 1);
        m >>= sh; e += sh;
        if (m >> 48) { m >>= 1; e++; }
    } else if (sh < 0) {
        m <<= -sh; e += sh;
    }
    if (e + 1024 <= 0) return 0;                       /* underflow */
    if (e + 1024 >= 0x7ff) return neg ? (~((u64)0x7ff << 48) & M60) : ((u64)0x7ff << 48);
    return fpack(neg, e, (u64)m);
}
static u64 fadd(u64 a, u64 b, int sub, int round)
{
    fl x = funpack(a), y = funpack(b);
    int e, d; __int128 sx, sy, s; u128 m;
    if (sub) y.neg = !y.neg;
    if (x.c == 0) { if (y.c == 0) return 0; x = y; return fnorm(x.neg, x.c, x.e, round); }
    if (y.c == 0) return fnorm(x.neg, x.c, x.e, round);
    if (x.e < y.e) { fl t = x; x = y; y = t; }
    d = x.e - y.e;
    if (d > 60) return fnorm(x.neg, x.c, x.e, round);
    e = y.e;
    sx = (__int128)((u128)x.c << d); if (x.neg) sx = -sx;
    sy = (__int128)y.c; if (y.neg) sy = -sy;
    s = sx + sy;
    if (s == 0) return 0;
    if (s < 0) { m = (u128)(-s); return fnorm(1, m, e, round); }
    return fnorm(0, (u128)s, e, round);
}
static u64 fmul(u64 a, u64 b, int round)
{
    fl x = funpack(a), y = funpack(b);
    u128 p = (u128)x.c * y.c;
    return fnorm(x.neg ^ y.neg, p, x.e + y.e, round);
}
/* integer multiply trick: DX product low half; ints have exponent field 0 */
static u64 dmul(u64 a, u64 b)
{
    fl x = funpack(a), y = funpack(b);
    u128 p = (u128)x.c * y.c;
    int neg = x.neg ^ y.neg;
    if (x.e == -1024 && y.e == -1024) {
        u64 r = (u64)(p & M48);
        return neg ? (~r & M60) : r;
    }
    {   /* real double precision low part */
        int e = x.e + y.e;
        u64 hi_c; int sh;
        u128 m = p; int pp = 127;
        if (m == 0) return 0;
        while (!((m >> pp) & 1)) pp--;
        sh = pp - 47; if (sh < 0) sh = 0;
        (void)hi_c;
        return fpack(neg, e + 0 - 0, (u64)(p & (((u128)1 << sh) - 1)) & M48);
    }
}
static u64 fdiv(u64 a, u64 b, int round)
{
    fl x = funpack(a), y = funpack(b);
    u128 q; int e;
    if (y.c == 0) return (u64)0x3ffULL << 48;        /* indefinite-ish */
    if (x.c == 0) return 0;
    /* normalise operands */
    while (!(x.c >> 47)) { x.c <<= 1; x.e--; }
    while (!(y.c >> 47)) { y.c <<= 1; y.e--; }
    q = ((u128)x.c << 50) / y.c;                      /* 49..51 bits */
    {
        u128 r = ((u128)x.c << 50) % y.c; if (r) q |= 0;  /* sticky ignored */
    }
    e = x.e - y.e - 50;
    return fnorm(x.neg ^ y.neg, q, e, round);
}
static double f2d(u64 w)
{
    fl f = funpack(w); double d = ldexp((double)f.c, f.e);
    return f.neg ? -d : d;
}


#include "cdcio.h"
#include "../nfs/netmodel.h"
#define DEVBASE 0120000          /* the Ethernet device window: 4112 words, see nfs/nfsdrv.pas */
#define IS_DEV(a) ((a) >= DEVBASE && (a) < DEVBASE + NET_DEVSIZE)
static void halt_exit(void)
{
    if (dumpf) { FILE *d = fopen(dumpf, "w"); unsigned q; for (q = 0; q < MEMW; q++) if (mem[q]) fprintf(d, "%06o %020llo\n", q, (unsigned long long)mem[q]); fclose(d); }
    io_closeall();
    fprintf(stderr, "[halt after %lld instructions]\n", ninstr);
    exit(0);
}
static u64 io_load(unsigned a)
{
    int c;
    if (a != IO_RX) return 0;
    c = next_input();
    return c < 0 ? M60 : (u64)c;
}
static u64 curfile;
static void io_store(unsigned a, u64 v)
{
    switch (a) {
    case IO_TX: io_putch((unsigned)(v & 63)); break;
    case IO_FILE: curfile = v & M60; break;
    case IO_FWORD: io_fword(curfile, v); break;
    case IO_HALT: halt_exit(); break;
    }
}
static int condtest(int c, u64 x)
{
    int e = (int)((x >> 48) & 0xfff);
    switch (c) {
    case 0: return x == 0 || x == M60;
    case 1: return !(x == 0 || x == M60);
    case 2: return !((x >> 59) & 1);
    case 3: return (x >> 59) & 1;
    case 4: return !(e == 03777 || e == 04000 || e == 01777 || e == 06000);
    case 5: return (e == 03777 || e == 04000 || e == 01777 || e == 06000);
    case 6: return !(e == 01777 || e == 06000);
    default: return (e == 01777 || e == 06000);
    }
}
static unsigned shiftcount18(unsigned b, int *left)
{
    if (b & 0x20000) { *left = 0; return (~b) & 077; }
    *left = 1; return b & 077;
}
static u64 rotl(u64 x, unsigned n)
{
    n %= 60; if (!n) return x;
    return ((x << n) | (x >> (60 - n))) & M60;
}
static u64 sar(u64 x, unsigned n)
{
    int s = (x >> 59) & 1;
    if (n >= 60) return s ? M60 : 0;
    if (!n) return x;
    x >>= n;
    if (s) x |= (M60 << (60 - n)) & M60;
    return x;
}

static void run(void)
{
    for (;;) {
        if (nm_uart_eof) halt_exit();
        u64 w; int slot;
        w = mem[P];
        for (slot = 0; slot < 4; ) {
            unsigned p0 = (unsigned)((w >> (45 - 15 * slot)) & 0x7fff);
            unsigned f = p0 >> 9, i = (p0 >> 6) & 7, j = (p0 >> 3) & 7, k = p0 & 7;
            unsigned K = 0; int jump = 0; unsigned target = 0; int longi = 0;
            if (tracei) fprintf(stderr, "T %o/%d\n", P, slot);
            if (++ninstr == maxinstr) { fprintf(stderr, "instr limit at P=%o slot=%d\n", P, slot); { int q; for (q=0;q<8;q++) fprintf(stderr,"X%d=%020llo A%d=%o B%d=%o\n",q,(unsigned long long)X[q],q,A[q],q,B[q]); } halt_exit(); }
            if (f == 1 || f == 2 || f == 3 || (f >= 4 && f <= 7) || f == 050 || f == 051 || f == 052 ||
                f == 060 || f == 061 || f == 062 || f == 070 || f == 071 || f == 072) {
                longi = 1;
                if (slot == 3) { fprintf(stderr, "30-bit instruction in last parcel at %o\n", P); halt_exit(); }
                K = ((k << 15) | (unsigned)((w >> (30 - 15 * slot)) & 0x7fff)) & M18;
                /* K is 18 bits: parcel1 is the low 15 bits, k is the top 3 */
                K = ((unsigned)(p0 & 7) << 15) | (unsigned)((w >> (30 - 15 * slot)) & 0x7fff);
                (void)longi;
            }
            switch (f) {
            case 0: fprintf(stderr, "PS executed at %o slot %d word %020llo X7=%o\n", P, slot, (unsigned long long)w, (unsigned)X[7]); halt_exit(); break;
            case 1: /* RJ */
                mem[K] = ((u64)04 << 54) | ((u64)0 << 48) | ((u64)((P + 1) & M18) << 30);
                /* upper half: EQ B0,B0,P+1 = 04 0 0 K (30 bits) */
                mem[K] = (((u64)(04 << 9) << 45) | ((u64)(P + 1) << 30)) & M60;
                jump = 1; target = K + 1; break;
            case 2: target = (K + (i ? B[i] : 0)) & M18; jump = 1; break;
            case 3: if (condtest((int)i, X[j])) { jump = 1; target = K; } break;
            case 4: if (sb18(B[i]) == sb18(B[j])) { jump = 1; target = K; } break;
            case 5: if (sb18(B[i]) != sb18(B[j])) { jump = 1; target = K; } break;
            case 6: if (sb18(B[i]) >= sb18(B[j])) { jump = 1; target = K; } break;
            case 7: if (sb18(B[i]) < sb18(B[j])) { jump = 1; target = K; } break;
            case 010: X[i] = X[j]; break;
            case 011: X[i] = X[j] & X[k]; break;
            case 012: X[i] = X[j] | X[k]; break;
            case 013: X[i] = X[j] ^ X[k]; break;
            case 014: X[i] = ~X[k] & M60; break;
            case 015: X[i] = ~X[k] & X[j] & M60; break;
            case 016: X[i] = (~X[k] | X[j]) & M60; break;
            case 017: X[i] = ~(X[k] ^ X[j]) & M60; break;
            case 020: X[i] = rotl(X[i], j * 8 + k); break;
            case 021: X[i] = sar(X[i], j * 8 + k); break;
            case 022: { int l; unsigned n = shiftcount18(B[j], &l); X[i] = l ? rotl(X[k], n) : sar(X[k], n); break; }
            case 023: { int l; unsigned n = shiftcount18(B[j], &l); X[i] = l ? sar(X[k], n) : rotl(X[k], n); break; }
            case 024: { /* NX */
                u64 x = X[k]; int neg = (x >> 59) & 1; unsigned cnt = 0;
                if (neg) x = ~x & M60;
                { u64 c = x & M48; int e = (int)((x >> 48) & 0xfff);
                  if (c == 0) { X[i] = 0; cnt = 0; }
                  else { while (!(c >> 47)) { c <<= 1; e--; cnt++; }
                    x = ((u64)(e & 0xfff) << 48) | c; X[i] = neg ? (~x & M60) : x; } }
                if (j) B[j] = cnt; break; }
            case 025: { /* ZX: round normalise */
                u64 x = X[k]; int neg = (x >> 59) & 1; unsigned cnt = 0;
                if (neg) x = ~x & M60;
                { u64 c = x & M48; int e = (int)((x >> 48) & 0xfff);
                  if (c == 0) X[i] = 0;
                  else { while (!(c >> 47)) { c <<= 1; e--; cnt++; }
                    x = ((u64)(e & 0xfff) << 48) | c; X[i] = neg ? (~x & M60) : x; } }
                if (j) B[j] = cnt; break; }
            case 026: { /* UX */
                u64 x = X[k]; int neg = (x >> 59) & 1; int e; u64 c;
                if (neg) x = ~x & M60;
                e = (int)((x >> 48) & 0xfff) - 1024; c = x & M48;
                if (j) B[j] = (e < 0) ? neg18((unsigned)(-e)) : (unsigned)e;
                X[i] = neg ? (~c & M60) : c; break; }
            case 027: { /* PX */
                u64 x = X[k]; int neg = (x >> 59) & 1; u64 c;
                if (neg) x = ~x & M60;
                c = x & M48;
                { int bj = (B[j] & 0x20000) ? -(int)neg18(B[j]) : (int)B[j];
                  u64 r = ((u64)((bj + 1024) & 0xfff) << 48) | c; X[i] = neg ? (~r & M60) : r; }
                break; }
            case 030: X[i] = fadd(X[j], X[k], 0, 0); break;
            case 031: X[i] = fadd(X[j], X[k], 1, 0); break;
            case 032: X[i] = fadd(X[j], X[k], 0, 0); break;   /* DX+ (approx) */
            case 033: X[i] = fadd(X[j], X[k], 1, 0); break;
            case 034: X[i] = fadd(X[j], X[k], 0, 1); break;
            case 035: X[i] = fadd(X[j], X[k], 1, 1); break;
            case 036: X[i] = add60(X[j], X[k]); break;
            case 037: X[i] = add60(X[j], ~X[k] & M60); break;
            case 040: X[i] = fmul(X[j], X[k], 0); break;
            case 041: X[i] = fmul(X[j], X[k], 1); break;
            case 042: X[i] = dmul(X[j], X[k]); break;
            case 043: { unsigned n = j * 8 + k; X[i] = n == 0 ? 0 : (n >= 60 ? M60 : ((((1ULL << n) - 1) << (60 - n)) & M60)); break; }
            case 044: X[i] = fdiv(X[j], X[k], 0); break;
            case 045: X[i] = fdiv(X[j], X[k], 1); break;
            case 046: break;
            case 047: X[i] = (u64)__builtin_popcountll(X[k]); break;
            case 050: A[i] = add18(K, A[j]); goto setA;
            case 051: A[i] = add18(K, B[j]); goto setA;
            case 052: A[i] = add18(K, (unsigned)(X[j] & M18)); goto setA;
            case 053: A[i] = add18(B[k], (unsigned)(X[j] & M18)); goto setA;
            case 054: A[i] = add18(B[k], A[j]); goto setA;
            case 055: A[i] = add18(neg18(B[k]), A[j]); goto setA;
            case 056: A[i] = add18(B[k], B[j]); goto setA;
            case 057: A[i] = add18(neg18(B[k]), B[j]); goto setA;
            setA:
                A[i] &= M18;
                if (i >= 1 && i <= 5) X[i] = IS_DEV(A[i]) ? net_read(A[i] - DEVBASE) : (A[i] >= IO_RX && A[i] <= IO_HALT) ? io_load(A[i]) : mem[A[i]];
                else if (i >= 6) { if (watch && A[i] == watch) fprintf(stderr, "WRITE %020llo to %o at P=%o slot %d (instr %lld)\n", (unsigned long long)X[i], A[i], P, slot, ninstr); if (IS_DEV(A[i])) net_write(A[i] - DEVBASE, X[i]); else if (A[i] >= IO_RX && A[i] <= IO_HALT) io_store(A[i], X[i]); else mem[A[i]] = X[i]; }
                break;
            case 060: if (i) B[i] = add18(K, A[j]); break;
            case 061: if (i) B[i] = add18(K, B[j]); break;
            case 062: if (i) B[i] = add18(K, (unsigned)(X[j] & M18)); break;
            case 063: if (i) B[i] = add18(B[k], (unsigned)(X[j] & M18)); break;
            case 064: if (i) B[i] = add18(B[k], A[j]); break;
            case 065: if (i) B[i] = add18(neg18(B[k]), A[j]); break;
            case 066: if (i) B[i] = add18(B[k], B[j]); break;
            case 067: if (i) B[i] = add18(neg18(B[k]), B[j]); break;
            case 070: X[i] = sx18(add18(K, A[j])); break;
            case 071: X[i] = sx18(add18(K, B[j])); break;
            case 072: X[i] = sx18(add18(K, (unsigned)(X[j] & M18))); break;
            case 073: X[i] = sx18(add18(B[k], (unsigned)(X[j] & M18))); break;
            case 074: X[i] = sx18(add18(B[k], A[j])); break;
            case 075: X[i] = sx18(add18(neg18(B[k]), A[j])); break;
            case 076: X[i] = sx18(add18(B[k], B[j])); break;
            case 077: X[i] = sx18(add18(neg18(B[k]), B[j])); break;
            default: fprintf(stderr, "bad opcode %o at %o\n", f, P); halt_exit();
            }
            B[0] = 0;
            if (jump && ninstr >= jlo && ninstr <= jhi) fprintf(stderr, "%lld: %o/%d -> %o  X7=%o X6=%llo\n", ninstr, P, slot, target, (unsigned)X[7], (unsigned long long)X[6]);
            if (jump) { P = target; break; }
            slot += (f == 1 || f == 2 || f == 3 || (f >= 4 && f <= 7) || f == 050 || f == 051 || f == 052 ||
                     f == 060 || f == 061 || f == 062 || f == 070 || f == 071 || f == 072) ? 2 : 1;
            if (slot >= 4) { P++; }
        }
        if (0) ;
    }
}


static void load_img(const char *f)
{
    FILE *fp = fopen(f, "r"); char line[128]; unsigned a; unsigned long long w;
    if (!fp) { perror(f); exit(2); }
    while (fgets(line, sizeof line, fp)) if (sscanf(line, "%o %llo", &a, &w) == 2) mem[a & M18] = w & M60;
    fclose(fp);
}

int main(int argc, char **argv)
{
    int a, nimg; unsigned xd = 0, xn = 0;
    if (argc < 3) { fprintf(stderr, "usage: cdcemu runtime.img program.img [-i in] [-o out] [-d bindir] [-n max]\n"); return 2; }
    init_chars();
    mem = calloc(MEMW, sizeof *mem);
    infp = stdin; outfp = stdout;
    for (a = 3; a < argc && argv[a][0] != '-'; a++) ;
    nimg = a;
    for (; a + 1 < argc; a += 2) {
        if (!strcmp(argv[a], "-i")) { infp = fopen(argv[a + 1], "rb"); if (!infp) { perror(argv[a + 1]); return 2; } }
        else if (!strcmp(argv[a], "-o")) outfp = fopen(argv[a + 1], "wb");
        else if (!strcmp(argv[a], "-d")) bindir = argv[a + 1];
        else if (!strcmp(argv[a], "-n")) maxinstr = atoll(argv[a + 1]);
        else if (!strcmp(argv[a], "-w")) watch = strtoul(argv[a + 1], 0, 8);
        else if (!strcmp(argv[a], "-t")) { tracei = atoi(argv[a + 1]); }
        else if (!strcmp(argv[a], "-r")) net_root = argv[a + 1];
        else if (!strcmp(argv[a], "-u")) nm_uart_in = fopen(argv[a + 1], "rb");
        else if (!strcmp(argv[a], "-m")) dumpf = argv[a + 1];
        else if (!strcmp(argv[a], "-x")) xd = strtoul(argv[a + 1], 0, 8);
        else if (!strcmp(argv[a], "-xn")) xn = atoi(argv[a + 1]);
        else if (!strcmp(argv[a], "-j")) { jlo = atoll(argv[a + 1]); jhi = atoll(argv[a + 2]); a++; }
    }
    for (a = 1; a < nimg; a++) load_img(argv[a]);
    P = mem[0] ? 0 : 0;
    /* boot: the program image names its entry in word 0 of the boot block (see pasclgo2img.py) */
    P = (unsigned)(mem[1] & M18); B[5] = (unsigned)(mem[2] & M18); B[6] = B[5];
    B[4] = (unsigned)(mem[3] & M18); X[7] = 0100;
    (void)xd; (void)xn;
    run();
    return 0;
}
