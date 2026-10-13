// Verilator harness for cdc6000_core: loads the runtime and a program image through the load port, feeds the
// input text, collects the character output and the binary file words.  No runtime lives here: GETC, PUTC, WRT*
// are CDC machine code in runtime.img (host/mkrt.py).
//
//   Vcdc6000_core runtime.img program.img [driver.img ...] [-i in] [-o out] [-d bindir] [-n maxcycles] [-m memdump] [-r nfsroot]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef CORE_SB
#define VTOP Vcdc6000_sb
#define VTOPH "Vcdc6000_sb.h"
#define VROOTH "Vcdc6000_sb___024root.h"
#define COREMEM cdc6000_sb__DOT__mem
#else
#define VTOP Vcdc6000_core
#define VTOPH "Vcdc6000_core.h"
#define VROOTH "Vcdc6000_core___024root.h"
#define COREMEM cdc6000_core__DOT__mem
#endif
#include VTOPH
#include VROOTH
#include "verilated.h"
#include "../host/cdcio.h"
#include "../nfs/netmodel.h"

static void load_into(VTOP *top, const char *f, void (*tick)(void))
{
    FILE *fp = fopen(f, "r"); char line[128]; unsigned a; unsigned long long w;
    if (!fp) { perror(f); exit(2); }
    while (fgets(line, sizeof line, fp))
        if (sscanf(line, "%o %llo", &a, &w) == 2) {
            top->load_we = 1; top->load_addr = a; top->load_data = w; tick();
        }
    fclose(fp);
    top->load_we = 0;
}

static int regdump, perf;
static VTOP *top;
static VerilatedContext *ctx;
static void tick(void)
{
    top->clk = 0; top->eval();
    top->clk = 1; top->eval();
}

int main(int argc, char **argv)
{
    long long maxcyc = 0, cycles = 0; const char *dumpf = nullptr; int a;
    ctx = new VerilatedContext;
    ctx->commandArgs(argc, argv);
    if (argc < 3) { fprintf(stderr, "usage: Vcdc6000_core runtime.img program.img [-i in] [-o out] [-d dir] [-n cycles]\n"); return 2; }
    init_chars();
    infp = stdin; outfp = stdout;
    for (a = 3; a < argc && argv[a][0] != '-'; a++) ;
    int nimg = a;
    for (; a + 1 < argc; a += 2) {
        if (!strcmp(argv[a], "-i")) { infp = fopen(argv[a + 1], "rb"); if (!infp) { perror(argv[a + 1]); return 2; } }
        else if (!strcmp(argv[a], "-o")) outfp = fopen(argv[a + 1], "wb");
        else if (!strcmp(argv[a], "-d")) bindir = argv[a + 1];
        else if (!strcmp(argv[a], "-n")) maxcyc = atoll(argv[a + 1]);
        else if (!strcmp(argv[a], "-m")) dumpf = argv[a + 1];
        else if (!strcmp(argv[a], "-r")) net_root = argv[a + 1];
        else if (!strcmp(argv[a], "-u")) nm_uart_in = fopen(argv[a + 1], "rb");
        else if (!strcmp(argv[a], "-D")) nm_dip = atoi(argv[a + 1]);
        else if (!strcmp(argv[a], "-N")) { nm_nodhcp = 1; a--; }
        else if (!strcmp(argv[a], "-R")) { regdump = 1; a--; }
        else if (!strcmp(argv[a], "-P")) { perf = 1; a--; }
    }
    top = new VTOP(ctx);
    top->reset = 1; top->load_we = 0; top->rx_valid = 0; top->tx_ready = 1; top->fo_ready = 1;
    top->dev_ack = 0; top->dev_rdata = 0;
    tick(); tick();
    for (int k = 1; k < nimg; k++) load_into(top, argv[k], tick);
    tick(); tick();
    top->reset = 0;

    // one character of lookahead for IO_RX
    int cur = next_input();
    auto present = [&]() {
        top->rx_valid = 1;
        top->rx_eof = cur < 0;
        top->rx_char = cur < 0 ? 0 : cur;
    };
    present();
    while (!nm_uart_eof && !top->halted) {
        tick(); cycles++;
        // the device window: the model answers a request in the cycle after it is raised
        if (top->dev_req && !top->dev_ack) {
            if (top->dev_we) net_write(top->dev_off, top->dev_wdata);
            else top->dev_rdata = net_read(top->dev_off);
            top->dev_ack = 1;
        } else top->dev_ack = 0;
        if (top->rx_take) { cur = next_input(); present(); }
        if (top->tx_valid) io_putch(top->tx_char);
        if (top->fo_valid) {
            uint64_t name = top->fo_name, word = top->fo_word;
            io_fword(name, word);
        }
        if (maxcyc && cycles >= maxcyc) { fprintf(stderr, "cycle limit, pc=%o\n", top->dbg_pc); break; }
    }
    if (dumpf) {
        FILE *d = fopen(dumpf, "w");
        auto &m = top->rootp->COREMEM;
        for (unsigned q = 0; q < (1u << 17); q++) if (m[q]) fprintf(d, "%06o %020llo\n", q, (unsigned long long)m[q]);
        fclose(d);
    }
    if (regdump) {                               // the same lines as cdcemu -R
        for (int q = 0; q < 8; q++) {
            unsigned long long xv = 0; unsigned av = 0, bv = 0;
            for (int w = 0; w < 60; w++) xv |= (unsigned long long)((top->dbg_x[(60 * q + w) / 32] >> ((60 * q + w) % 32)) & 1) << w;
            for (int w = 0; w < 18; w++) {
                av |= ((top->dbg_a[(18 * q + w) / 32] >> ((18 * q + w) % 32)) & 1) << w;
                bv |= ((top->dbg_b[(18 * q + w) / 32] >> ((18 * q + w) % 32)) & 1) << w;
            }
            printf("X%d=%020llo A%d=%06o B%d=%06o\n", q, xv, q, av, q, bv);
        }
    }
#ifdef CORE_SB
    if (perf) {
        static const char *nm[14] = {"cycles", "issued", "decode register empty", "held for a branch", "unit busy", "destination reserved",
            "no tag", "memory queue full", "waiting to drain", "redirects", "decode waited for a word", "empty within 3 cycles of a redirect",
            "memory head waits for its address", "memory head waits for store data"};
        for (int q = 0; q < 14; q++) {
            uint32_t v = 0;
            for (int w = 0; w < 32; w++) v |= ((top->dbg_perf[(32 * q + w) / 32] >> ((32 * q + w) % 32)) & 1) << w;
            fprintf(stderr, "  %-38s %10u\n", nm[q], v);
        }
    }
#endif
    io_closeall();
    fprintf(stderr, "[halt after %llu instructions, %lld cycles%s]\n", (unsigned long long)top->icount, cycles,
            top->fault ? ", FAULT" : "");
    top->final();
    delete top;
    return top ? 0 : 0;
}
