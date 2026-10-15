# CDC 6000 core

`cdc6000_core.sv` + `cdc_pkg.sv`: the CDC 6000 instruction subset the 1972 Pascal compiler generates (integer,
shift, mask, pack/unpack/normalise, 60-bit floating point, branches, SA/SB/SX), built on the structure of the
OCaml bytecode core in `template/` (single FSM, synchronous memory, load port while in reset, strobe outputs).

* Word length 60 bits; X registers 60 bits, A/B 18 bits, memory `2**MEM_AW` words (default 128K).
* Floating point is the CDC format (11-bit exponent, 48-bit coefficient, negation = complement of the word), not
  IEEE: `cdc_pkg.sv` transcribes the model in `../host/cdcemu.c`.  Add/multiply are combinational, divide is a
  51-cycle restoring divider.  They are the long paths: pipeline them before running above ~25 MHz.
* No runtime in C and none in the harness: GETC/PUTC/WRT* are CDC machine code (`../host/mkrt.py`), loaded with the
  program.  The core's I/O is five memory-mapped words (020B-024B) surfaced as rx/tx/file-out ports.
* `make` builds `obj_dir/Vcdc6000_core` (harness `main_cdc.cpp`); `./selfhost_rtl.sh` runs the compiler on itself
  on the core and compares with the reference.  `make clean` removes the build.

Untested: synthesis (no yosys/Vivado run yet), the memory-mapped I/O behind a real UART, backpressure
(`tx_ready`/`fo_ready` are tied high in the harness).

## The scoreboarded core and performance

`cdc6000_sb.sv` is a second core with the same instruction set and the same ports: an instruction stack, in-order issue,
a scoreboard (reservations and operand capture from the result buses), separate units (boolean, shift, long add, floating add,
multiply, divide, normalise, increment, branch), an ordered memory queue and a branch history table.  Design and
measurements: `SUPERSCALAR.md`.  `make sb` builds `obj_dir_sb/Vcdc6000_sb`; `../tests/sbregress.sh` is the full regression
(random programs against the C model, unit-count variants, floating point, the compiler fixed point, the NFS self-compile).
On the board `build.tcl` selects it with `VM_DEFS="CORE_SB=1 SYS_DIV=10.000 CLK_HZ=100000000"`; `VM_SYNTH="-retiming"
VM_PLACE=ExtraTimingOpt` adds synthesis retiming and the timing-driven placer directive.

Measured on the VC707 (`bench` and `compile pascal` in the shell, times in ms; the compile is the 1972 compiler compiling its
own source with all run time checks on -- the compiler has no optimiser).  The in-order column is the scoreboarded core
before speculation and dual issue (commit d48f519 and earlier, WNS +0.18 ns at 100 MHz); the last two are the current core
(speculation past branches, dual issue, SUPERSCALAR.md section 10), which closes 95.2 MHz with the speed synthesis set and
100 MHz with the overconstrained placement and chained post-route passes of section 11 (2026-10-15):

| | sequential core, 50 MHz | scoreboarded, in order, 100 MHz | dual issue, 95.2 MHz | dual issue, 100 MHz | speed-up |
|---|---|---|---|---|---|
| `bench` sieve, 20 x 1000 | 70 | 18 | 12 | 11 | 6.4x |
| `bench` arithmetic loop (200000 DIV/MOD) | 692 | 328 | 336 | 320 | 2.2x |
| `compile pascal` (includes NFS transfers) | 12359 | 4752 | 3406 | 3299 | 3.7x |
| cycles per instruction (simulated self-compile) | 2.12 | 1.13 | 0.70 | 0.70 | 3.0x |
| cycles, self-compile of the compiler | 73.8 M | 42.4 M | 26.3 M | 26.3 M | 2.81x |
| LUTs (whole board design) | 7.5 K (2.5 %) | 26.9 K (8.9 %) | 42.3 K (13.9 %) | 42.3 K (13.9 %) | |

The arithmetic loop gains only the clock: the divide unit takes the same number of cycles on all three cores.  The 95.2 MHz
column is the build with the UART fix of 090b058 (0.08 ns short on paper, runs); the 100 MHz column's compile time is from
the build 0.20 ns short, which runs at the same cycle count as the closed one (the closed build measured the same bench).
Of the compile's 3.3 s about 0.26 s is the compiler itself: the rest is the NFS driver (Pascal, compiled by the same
compiler) moving the source, the listing and the object through the Ethernet window a byte at a time, and the server's
round trips.  The compile time over NFS varies with the server (3299 to 4932 ms were seen in one morning); on the ramdisk
(nfs/README.md: block RAM in the FPGA holding the source and the compiler image, `ram` in the shell) it is **2785 ms at
95.2 MHz, the same to the millisecond every time**, compiler image reload from the ramdisk included.  The 650 ms
difference is the network's share; the rest is the run time taking the files from the driver a character a call.

## Device bus and the Ethernet board
Loads and stores to 120000B..+4111 leave the core on `dev_req/dev_we/dev_off/dev_wdata`, `dev_ack/dev_rdata` (state `S_DEV`).
`../fpga/vc707-eth/cdc_eth_core.v` answers them with the ethmin packet windows and registers; `../nfs/netmodel.h` answers them
(plus a model of the server behind them) in the Verilator bench and the C model.  Floating add/multiply/divide share one multiplier
and one normaliser, finishing in state `S_FPN`.
