/* Character set and file I/O shared by the C reference model (cdcemu.c) and the RTL harness (rtl/main_cdc.cpp).
 * Characters are display code (A=1 .. Z=26, 0-9 = 27..36, space = 45, line end = 0). */
#ifndef CDCIO_H
#define CDCIO_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
/* ---------- the outside world ---------- */
static int fromascii_[256];
static char toascii_[64];
static FILE *infp, *outfp;
static const char *bindir = ".";
static FILE *binfp[64]; static uint64_t binname[64]; static int nbin;
static void init_chars(void)
{
    int k;
    for (k = 0; k < 256; k++) fromascii_[k] = 45;
    for (k = 0; k < 26; k++) { fromascii_['A' + k] = k + 1; fromascii_['a' + k] = k + 1; }
    for (k = 0; k < 10; k++) fromascii_['0' + k] = 27 + k;
    fromascii_['+'] = 37; fromascii_['-'] = 38; fromascii_['*'] = 39; fromascii_['/'] = 40;
    fromascii_['('] = 41; fromascii_[')'] = 42; fromascii_['$'] = 43; fromascii_['='] = 44;
    fromascii_[' '] = 45; fromascii_[','] = 46; fromascii_['.'] = 47; fromascii_['['] = 49;
    fromascii_[']'] = 50; fromascii_[':'] = 51; fromascii_['<'] = 58; fromascii_['>'] = 59;
    fromascii_[';'] = 63; fromascii_['\''] = 48; fromascii_['^'] = 56;
    for (k = 0; k < 64; k++) toascii_[k] = '?';
    for (k = 0; k < 26; k++) toascii_[k + 1] = 'A' + k;
    for (k = 0; k < 10; k++) toascii_[27 + k] = '0' + k;
    toascii_[0] = '~';
    toascii_[37] = '+'; toascii_[38] = '-'; toascii_[39] = '*'; toascii_[40] = '/';
    toascii_[41] = '('; toascii_[42] = ')'; toascii_[43] = '$'; toascii_[44] = '=';
    toascii_[45] = ' '; toascii_[46] = ','; toascii_[47] = '.'; toascii_[48] = '\'';
    toascii_[49] = '['; toascii_[50] = ']'; toascii_[51] = ':'; toascii_[52] = '#';
    toascii_[53] = '@'; toascii_[54] = '|'; toascii_[55] = '&'; toascii_[56] = '^';
    toascii_[57] = '!'; toascii_[58] = '<'; toascii_[59] = '>'; toascii_[60] = '{';
    toascii_[61] = '}'; toascii_[62] = '~'; toascii_[63] = ';';
}
/* the input as the machine sees it: display code, line end = 0; UTF-8 glyphs of the 1972 character set are decoded */
static int next_input(void)
{
    int c, b1 = 0, b2 = 0;
    c = fgetc(infp);
    if (c == EOF) return -1;
    if (c == '\n') return 0;
    if (c == 226 || c == 194) {
        if (c == 226) b1 = fgetc(infp);
        b2 = fgetc(infp);
        if (c == 194) return 62;
        if (b1 == 136 && b2 == 167) return 55;
        if (b1 == 136 && b2 == 168) return 54;
        if (b1 == 137 && b2 == 160) return 52;
        if (b1 == 137 && b2 == 164) return 60;
        if (b1 == 137 && b2 == 165) return 61;
        if (b1 == 137 && b2 == 161) return 48;
        if (b1 == 134 && b2 == 145) return 56;
        if (b1 == 134 && b2 == 147) return 57;
        if (b1 == 134 && b2 == 146) return 53;
        return 45;
    }
    return fromascii_[c & 255];
}

static void io_putch(unsigned c)
{
    if ((c & 63) == 0) fputc('\n', outfp); else fputc(toascii_[c & 63], outfp);
}
/* one word of the binary file called `name` (an LFN word); files are created on first use as bindir/lowercase.out */
static void io_fword(uint64_t name, uint64_t v)
{
    int k; char nm[16], path[300];
    for (k = 0; k < nbin; k++) if (binname[k] == name) break;
    if (k == nbin) {
        int q, n = 0;
        for (q = 0; q < 10; q++) { int c = (name >> (54 - 6 * q)) & 63; if (c) nm[n++] = tolower((unsigned char)toascii_[c]); }
        nm[n] = 0;
        snprintf(path, sizeof path, "%s/%s.out", bindir, nm);
        binfp[k] = fopen(path, "w"); binname[k] = name; nbin++;
        if (!binfp[k]) { perror(path); exit(2); }
    }
    fprintf(binfp[k], "%020llo\n", (unsigned long long)(v & ((1ULL << 60) - 1)));
}
static void io_closeall(void)
{
    int k;
    fflush(outfp);
    for (k = 0; k < nbin; k++) fclose(binfp[k]);
}
#endif
