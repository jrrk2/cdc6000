// Testbench for cdc_eth_core: the whole board minus the PHY.  The MAC stream is driven with the frames of the server model
// (nfs/netmodel.h), so the DMA, the packet RAM, the registers and the core run the NFS driver exactly as the board will.
//   Vcdc_eth_core -r exportdir [-n maxcycles] [-u uartinput]
// The UART is simulated at the bit level (434 clocks a bit, as on the board): -u gives the bytes sent to the board, what the
// board sends is printed on stdout.  With -u the run ends when the input is used up and the board has been quiet for a while.
#include "Vcdc_eth_core.h"
#include "Vcdc_eth_core___024root.h"
#include "verilated.h"
#include "../../../nfs/netmodel.h"
#include <vector>

int main(int argc, char **argv)
{
    VerilatedContext *ctx = new VerilatedContext;
    ctx->commandArgs(argc, argv);
    long long maxcyc = 0, cycles = 0;
    std::vector<uint8_t> uin; bool useu = false;
    for (int a = 1; a + 1 < argc; a += 2) {
        if (!strcmp(argv[a], "-u")) {
            FILE *f = fopen(argv[a + 1], "rb"); int c; useu = true;
            while (f && (c = fgetc(f)) != EOF) uin.push_back(c);
            if (f) fclose(f);
        } else if (!strcmp(argv[a], "-r")) net_root = argv[a + 1];
        else if (!strcmp(argv[a], "-n")) maxcyc = atoll(argv[a + 1]);
    }
    auto *top = new Vcdc_eth_core(ctx);
    top->UART_RX = 1; top->DIP = 0; top->BTN = 0; top->pcspma_status = 0x0303;
    top->rx_axis_tvalid = 0; top->rx_axis_tlast = 0; top->rx_axis_tuser = 0; top->tx_axis_tready = 1;
    top->resetn = 0; top->eth_rst = 1;
    auto tick = [&]() {
        top->clk_sys = 0; top->eth_clk = 0; top->eval();
        top->clk_sys = 1; top->eth_clk = 1; top->eval();
    };
    for (int i = 0; i < 20; i++) tick();
    top->resetn = 1; top->eth_rst = 0;
    std::vector<uint8_t> out;          // frame being sent by the board
    uint8_t rf[1536]; int rlen = 0, rpos = 0;      // frame being fed to the board
    const int BIT = 434;
    size_t upos = 0; int ubit = -1, ucnt = 0; unsigned ushift = 0;     // transmit to the board: ubit -1 idle, 0 start, 1..8 data, 9 stop
    int rbit = -1, rcnt = 0; unsigned rbyte = 0; long long quiet = 0;   // receive from the board
    while (!top->halted && !(maxcyc && cycles >= maxcyc)) {
        if (useu) {
            if (ubit < 0) {
                if (upos < uin.size() && cycles > 2000000) { ushift = uin[upos++]; ubit = 0; ucnt = BIT; top->UART_RX = 0; }
            } else if (--ucnt == 0) {
                ucnt = BIT; ubit++;
                if (ubit <= 8) top->UART_RX = (ushift >> (ubit - 1)) & 1;
                else if (ubit == 9) top->UART_RX = 1;
                else { ubit = -1; ucnt = 0; }
            }
            if (rbit < 0) { if (!top->UART_TX) { rbit = 0; rcnt = BIT + BIT / 2; rbyte = 0; } }
            else if (--rcnt == 0) {
                rcnt = BIT;
                if (rbit < 8) rbyte |= top->UART_TX << rbit++;
                else { putchar(rbyte); fflush(stdout); rbit = -1; quiet = 0; }
            }
            quiet++;
            if (upos >= uin.size() && ubit < 0 && quiet > 30000000) break;
        }
        // board -> server
        if (top->tx_axis_tvalid && top->tx_axis_tready) {
            out.push_back(top->tx_axis_tdata);
            if (top->tx_axis_tlast) { net_push_frame(out.data(), (int)out.size()); out.clear(); }
        }
        // server -> board: one frame at a time, when the RX window is free
        top->rx_axis_tvalid = 0; top->rx_axis_tlast = 0;
        if (rlen == 0 && !top->rootp->cdc_eth_core__DOT__rx_valid) rlen = net_pop_reply(rf), rpos = 0;
        if (rlen) {
            top->rx_axis_tdata = rf[rpos]; top->rx_axis_tvalid = 1; top->rx_axis_tlast = (rpos == rlen - 1);
            if (++rpos == rlen) rlen = 0;
        }
        tick(); cycles++;
    }
    fprintf(stderr, "[%s after %lld cycles, %ld frames sent, %ld received, fault=%d]\n", top->halted ? "halted" : "cycle limit", cycles,
            nm_frames_tx, nm_frames_rx, top->fault);
    delete top;
    return top ? 0 : 0;
}
