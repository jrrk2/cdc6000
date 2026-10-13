// Verilator testbench for vc707_cdc_top: plays the host.  Sends the input (stdin or -i file, converted to display code),
// honours the credit protocol, decodes the byte stream coming back into a listing and the binary files.
//   Vvc707_cdc_top -i pascal.cdc -o listing.txt -d outdir
#include "Vvc707_cdc_top.h"
#include "verilated.h"
#include "../../../host/cdcio.h"
#include <vector>
#include <deque>

int main(int argc, char **argv)
{
    VerilatedContext *ctx = new VerilatedContext;
    ctx->commandArgs(argc, argv);
    init_chars();
    infp = stdin; outfp = stdout;
    for (int a = 1; a + 1 < argc; a += 2) {
        if (!strcmp(argv[a], "-i")) infp = fopen(argv[a + 1], "rb");
        else if (!strcmp(argv[a], "-o")) outfp = fopen(argv[a + 1], "wb");
        else if (!strcmp(argv[a], "-d")) bindir = argv[a + 1];
    }
    auto *top = new Vvc707_cdc_top(ctx);
    const int bit = 25000000 / 921600 - 0;       // must agree with the DEFAULT_DIV+1 clocks per bit: 27
    const int BIT = 27; (void)bit;
    std::deque<uint8_t> txq;                     // bytes to send to the board
    size_t sent = 0; bool eof_queued = false;
    auto feed = [&](int n) {
        while (n-- > 0 && !eof_queued) {
            int c = next_input();
            if (c < 0) { txq.push_back(0x80); eof_queued = true; } else txq.push_back((uint8_t)c);
        }
    };
    // serial transmit state (host -> board)
    int tx_cnt = 0, tx_bits = 0; unsigned tx_shift = 0; int rxline = 1;
    // serial receive state (board -> host)
    int last = 1, rs = -1, rc = 0, rb = 0;
    // frame decoding
    int fstate = 0; int fneed = 0; uint64_t fa = 0, fb = 0; int fbytes = 0; uint8_t rep[17]; int repn = 0;
    bool ready = false, done = false; long credits = 0; uint64_t cyc = 0;
    top->cpu_reset = 0; top->serial_rx = 1;
    for (; cyc < 3000000000ULL && !done; cyc++) {
        top->clk200_p = 0; top->clk200_n = 1; top->eval();
        top->clk200_p = 1; top->clk200_n = 0; top->eval();
        // ---- host -> board
        if (tx_bits == 0 && ready && !txq.empty() && tx_cnt == 0) {
            tx_shift = (1u << 9) | ((unsigned)txq.front() << 1); txq.pop_front();   // stop, data, start(0)
            tx_bits = 10; tx_cnt = BIT; rxline = tx_shift & 1;
        }
        if (tx_bits) {
            if (--tx_cnt == 0) {
                tx_shift >>= 1; tx_bits--; tx_cnt = BIT;
                rxline = tx_bits ? (tx_shift & 1) : 1;
                if (tx_bits == 0) tx_cnt = 0;
            }
        }
        top->serial_rx = rxline;
        // ---- board -> host
        int t = top->serial_tx;
        if (rs < 0) { if (last && !t) { rs = 0; rc = BIT + BIT / 2; rb = 0; } }
        else if (--rc == 0) {
            if (rs < 8) { rb |= t << rs; rs++; rc = BIT; }
            else {
                rs = -1;
                int b = rb & 255;
                if (fstate == 0) {
                    if (b < 64) io_putch(b);
                    else if (b == 0x81) { fstate = 1; fneed = 16; fbytes = 0; fa = fb = 0; }
                    else if (b == 0x82) { fstate = 2; fneed = 17; repn = 0; }
                    else if (b == 0x83) { credits++; feed(256); }
                    else if (b == 0x85) { ready = true; feed(1024); }
                } else if (fstate == 1) {
                    if (fbytes < 8) fa = (fa << 8) | b; else fb = (fb << 8) | b;
                    if (++fbytes == 16) { io_fword(fa & ((1ULL << 60) - 1), fb); fstate = 0; }
                } else {
                    rep[repn++] = b;
                    if (repn == 17) {
                        uint64_t ic = 0, cy = 0;
                        for (int i = 0; i < 8; i++) { ic = (ic << 8) | rep[i]; cy = (cy << 8) | rep[8 + i]; }
                        fprintf(stderr, "[board halted: %llu instructions, %llu cycles%s]\n", (unsigned long long)ic,
                                (unsigned long long)cy, rep[16] ? ", FAULT" : "");
                        done = true;
                    }
                }
            }
        }
        last = t;
    }
    io_closeall();
    fprintf(stderr, "[tb: %llu clock cycles, %ld credits, leds=0x%02x]\n", (unsigned long long)cyc, credits, top->leds);
    delete top;
    return done ? 0 : 1;
}
