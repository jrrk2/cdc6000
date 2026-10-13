# From the sequential core to a scoreboarded core with several functional units

This is a plan, with the measurements it rests on.  Nothing in it is built yet except the measuring tools
(`host/cdcemu -T`, `host/scoreboard.c`).

## 1  What is wanted, and what is not

* Keep the instruction set exactly as it is: 15 and 30 bit parcels, four to a 60 bit word, X0-7, A0-7, B0-7, the same
  floating point.  The 1972 compiler, the run time and everything compiled stay binary compatible, and the C model
  `host/cdcemu.c` stays the reference of what the instructions do.
* Make the organisation like the machine this instruction set came from: an instruction stack, in-order issue, a set of functional units
  (boolean, shift, floating add, long add, multiply, divide, increment, branch), and a **scoreboard** that lets an
  instruction wait inside its unit for its operands while later instructions for other units go on.  That organisation is
  described in the open literature (J. E. Thornton, *Design of a Computer: the Control Data 6600*, 1970, and later
  papers); this plan and any code that follows are written from that description and from the measurements below, not from
  any vendor's documents, schematics or source.
* Not copied, because nothing in this project needs it: the exchange jump and monitor mode, the peripheral processors,
  the 32 bank central memory with its stunt box, and the real machine's unit latencies (they belonged to 1964 logic).  The
  device window stays our I/O.

## 2  The present core

`rtl/cdc6000_core.sv` is one state machine: fetch a word (two cycles: `S_FETCH`, `S_FETCHW`), then each parcel in
`S_EXEC` (one cycle), plus a cycle for a load (`S_LDW`) or a floating operation (`S_FPN`), 52 for a division.  It does a whole
instruction in a cycle, so its clock is slow: the board build at 50 MHz has 1.0 ns of slack, i.e. a critical path near 19 ns.

On the compile of the compiler (`host/selfhost.sh`, 34.8 M instructions) it takes 73.8 M cycles: 2.12 cycles an
instruction, **23.6 MIPS**.

## 3  Measurements

`cdcemu ... -T file` writes a 4 byte record for every executed instruction.  `scoreboard` replays it under a model: in-order
issue (1 or more a cycle), functional units with a count, latency and busy time, a reservation on every destination register
(an instruction does not issue while an earlier one has not written the same register), operands waited for inside the unit,
a result held back while an earlier instruction still has to read the old value, one memory port, and a taken-branch penalty.
`mode=seq` is the present core.  **Calibration:** the model gives 73,812,688 cycles for the self-compile, the RTL needs about
73.8 M.

Instruction mix (share of instructions):

| | self-compile (UART run time) | NFS driver run (first 60 M) |
|---|---|---|
| address/index arithmetic (A, B, X := address forms) | 49.8 % | 48.4 % |
| branches | 17.5 % (71 % of them taken) | 14.6 % (51 % taken) |
| boolean | 8.4 % | 14.3 % |
| long add | 6.4 % | 9.5 % |
| shift | 5.2 % | 3.9 % |
| no-operation (padding) | 12.7 % | 9.3 % |
| loads / stores | 24 % / 8 % | 18 % / 9 % |
| floating point, multiply, divide | ~0 | ~0 |

Speed-up in cycles over the present core, same instructions (`host/scoreboard`, parameters in the sweep below):

| | self-compile | NFS driver |
|---|---|---|
| present execute stage behind a front end only (instruction stack, no-operations dropped, taken branch 2) | **1.55x** | **1.63x** |
| A  pipelined, interlocked, no scoreboard (load 2+2, taken branch 2) | 1.20x | 1.34x |
| B  A with a scoreboard (units wait for operands) | 1.25x | 1.40x |
| C  B with taken branch 1 cycle | 1.35x | 1.48x |
| D  C with the load one cycle shorter | 1.50x | 1.61x |
| E  C with two instructions a cycle | 1.44x | 1.59x |
| F  E with three increment units | 1.48x | 1.65x |
| ideal: every latency 1, free branches, four instructions a cycle | 1.85x | 2.07x |

(Rows are not all cumulative: E and F build on C.  Unit latencies in A to F: boolean 1, shift 2, long add 2, floating add 4,
multiply 4 (pipelined), divide 52, increment 2 units of 2 cycles; load 2 more cycles after the address.)

What this says:

1. **This code has little instruction-level parallelism.**  Compiled Pascal is dependent address arithmetic, loads that feed
   the next instruction, and a taken branch every seven instructions.  Even an ideal four-wide machine reaches under 2x in cycles.
   A scoreboard on its own is worth only a few per cent over a plain interlocked pipeline here (B against A).  It pays on
   floating point, multiplication and division, which this workload does not use (the compiler is integer only), and it is what
   allows several slow units to overlap.
2. **The cheap, large wins are not superscalar:** the front end (no fetch cycles, padding skipped, branches into the
   stack) is 1.55-1.63x with the present execute stage untouched; shaving the taken-branch cost and the load-to-use time
   are the next largest.
3. **The largest win is the clock.**  The present path is a whole instruction in one 19 ns cycle.  Split into stages (decode,
   operand read, execute in a unit, write back) the paths are an 60 bit add, a barrel shift, a BRAM read: I expect 100-150 MHz on
   this device (to be measured on each block before relying on it).  Throughput is clock x instructions per cycle:

   | | clock | cycles per instruction | MIPS |
   |---|---|---|---|
   | now | 50 MHz | 2.12 | 23.6 |
   | front end only | 50 MHz | 1.36 | 37 |
   | pipelined + scoreboard, C | 100 MHz | 1.57 | 64 |
   | the same | 150 MHz | 1.57 | 96 |
   | D | 150 MHz | 1.41 | 106 |

   (instructions counted as the trace counts them, padding included.)

## 4  Target organisation

```
 memory (BRAM, 64K x 60, true dual port)
   port B: instruction words        port A: loads and stores (the A-register references)
        |                                  ^
  instruction stack (8 words, tags)        |
        |  parcels                         |
   predecode / issue (in order, 1 a cycle, later 2) -- no-operations dropped, 30 bit forms joined
        |
   scoreboard: one reservation per register (X0-7, A0-7, B0-7) naming the unit that will write it,
               one entry per unit: busy, op, destination, sources, "operand ready" flags, which unit makes each source
        |
   units:  boolean | shift/normalise | floating add | long add | multiply (DSP48, pipelined) | divide | increment x2-3 | branch
        |                                  |
   result paths to the X, A, B registers (write-port arbitration; a unit that cannot write holds its result)
```

Rules (the scoreboard, in words):

* **Issue** of an instruction waits while its unit is busy or its destination register is reserved by an unfinished
  instruction.  Otherwise the unit takes it, the destination is reserved for that unit, and issue goes on to the next one.
* **Read operands**: a source register that is not reserved is read at once; a reserved one is waited for in the unit and taken
  from the producing unit's result when it appears.
* **Write result**: held until every earlier instruction that names the same register as a source has read it, then written,
  and the reservation cleared.
* **A registers**: setting A1-A5 starts a load into X1-X5 (one memory port, in order); A6-A7 a store of X6-X7.  Loads and
  stores keep program order, so memory needs no further checking.  A store to a word in the instruction stack invalidates it
  (the `RJ` return-jump word is such a store).
* **Branches** are resolved in the branch unit; issue goes on past a conditional branch as if it were not taken and is
  squashed if it was (no results are written before the branch resolves: instructions after it may issue but their units hold the
  results until it is resolved).  A one-word-deep loop held in the stack costs one cycle per iteration.
* **Exceptions** (`PS`, a bad opcode, a 30 bit instruction in the last parcel) stop the machine as now; the state is not made
  precise.
* **Floating point** stays one shared normaliser per class of unit; the divide unit stays the 51 step restoring divider and
  holds its issue slot.

All of this is a parameterised module (unit count, latency, busy time, stack depth, issue width) so that the sweep in section 3
can be repeated on the RTL.

## 5  Stages

Each stage ends with everything below passing: the float differential test (`tests/fptest.py`), the fixed point
(`rtl/selfhost_rtl.sh`), the NFS self-compile (`nfs/selfhost_nfs.sh rtl`), the shell sessions, the board-level simulation, and a
measured cycle count on the self-compile, which is compared with the model.

0. **Done:** the trace (`cdcemu -T`), the timing model, this document.
1. **Measure the RTL itself and build the safety net.**  Cycle and instruction counters readable from the program;
   a retire trace out of the RTL (`+trace` already prints `P/slot`) extended with the register and memory writes, compared with
   the C model's; a random program generator (straight-line and small loops, all unit kinds, no I/O) whose final registers
   and memory are compared between the C model, the present core and each new core.  Model calibration against the RTL.
2. **Front end on the present execute stage.**  Instruction stack (eight words, tags, fed from BRAM port B), predecode that
   skips no-operations, a taken branch to a word in the stack costs one cycle.  Expected 1.55-1.63x at 50 MHz.  Cheapest and
   safest, and it is also the front end the later stages use.  Needs the BRAM wrapper's port B (`fpga/vc707-eth/genmem.py`) and a
   second read port in `cdc_ram_bram`.
3. **Pipelined execute with interlocks (no scoreboard yet).**  Stages decode, read, execute, write; stalls on a source that is
   not written yet.  Aim: the clock.  First step is to synthesise the arithmetic blocks alone (60 bit add, shifter, 18 bit
   increment, the shared normaliser, the multiplier) to see what clock they allow, then set the target: expected 100 MHz.
   Everything with a clock of its own (UART divider, millisecond counter, device handshake with the Ethernet side) is re-timed.
4. **Scoreboard and several units.**  Turn the interlock into the reservation and per-unit operand wait above; separate
   boolean, shift, long add, floating add, pipelined multiply, divide and increment units; result path arbitration.  Expected
   gain in cycles: a few per cent on the compiler, large on floating point, and the structure the next stages need.
5. **Branches.**  Loop capture in the instruction stack, issue past conditional branches with squash, backward-taken guess.
   Model: taken branch from 2 to 1 cycle is +8 % (C against B).
6. **Memory.**  Shorten load-to-use: BRAM output register removed from the load path if the clock allows, early forwarding from
   the increment unit to the load address.  Model: one cycle is +11 %.
7. **Two instructions a cycle (optional).**  The model says +6 % at best on this code; do it last, if at all.

## 6  Verification

* The C model remains the specification.  Every new core is run against it, not only against the old RTL.
* Added assertions in simulation: no register has two reservations, a unit never writes a register that another unit is to write
  later in program order, nothing is written before an earlier reader has read, no result of a squashed instruction reaches a register.
* The random generator (stage 1) is the main tool for hazards: dependent chains, load then use, store then load of the same
  word, branches taken and not taken around units that are still busy.
* The timing model is re-run on every stage with that stage's real latencies; a difference of more than a few per cent between
  model and RTL is a bug in one of them.

## 7  Risks and open questions

* The clock estimates are guesses until blocks are synthesised.  The floating normaliser is the longest path today (one shared
  unit, `S_FPN`); at 100 MHz or more it has to be pipelined, which the compiler workload will not notice.
* Instruction words that are stored into (the `RJ` word, programs that patch themselves) must be handled by the stack
  invalidation; the compiler and the run time do not patch other code.
* The Ethernet side runs on its own 125 MHz clock; the device window crossing is a handshake and should survive a faster core
  clock, but the UART divider and the millisecond counter take `CLK_HZ` as a parameter and must be set with it.
* A scoreboard that is correct for the compiler can still be wrong for code that uses the less common units (normalise, pack,
  unpack, mask, count, the double-precision forms): the random generator must cover them, and the float differential test already
  covers the arithmetic.

## 8  To repeat the measurements

```
host/cdcemu host/runtime.img host/stage2/in.img -i host/pascal.cdc -o /tmp/l.txt -d /tmp -T /tmp/selfhost.trace
gcc -O2 -o host/scoreboard host/scoreboard.c
host/scoreboard mode=seq < /tmp/selfhost.trace
host/scoreboard mode=sb inorder=1 memlat=2 brpen=2 spec=1 dropno=1 increment=2,1,1 shift=1,2,1 long_add=1,2,1 < /tmp/selfhost.trace
```
(units are `boolean shift float_add long_add multiply divide increment branch`, each `count,latency,busy`; other keys:
`width memlat brpen spec dropno inorder fwd limit`.)
