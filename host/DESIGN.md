# CDC 6000 Pascal bootstrap: ABI derived from the compiler source

All of this is read off `ocr/pascal.pas` (the 1972 compiler); the runtime that the
original shipped (COMPASS) is not in the listing, so it has to be rewritten.

## Memory image
* Absolute addresses, relative to RA.  `PARMLIST.ICO` = address of a cell holding the saved
  B5; compiled code starts at `ICO+1`.  PARMLIST (11 words, ICO+1..): ICO, ERRFLAG, ERRNRS[0..3],
  LOADPT, LIMCODE, LPJMPTAB, EXTFLAGS, ENTRYPT.
* Variables occupy `ICO+1 .. LOADPT` (BSS, never loaded).  The PASCLGO stream holds words for
  `LOADPT .. LIMCODE` followed by 50 extra words = the real jump table, which must be copied to
  `LPJMPTAB ..` (a 50-word hole inside the stream when VALUE data exists).
* Start: jump to ENTRYPT with B0=0, B5 = frame base (just above the image), B4 = stack limit,
  X7 = return address (to the runtime), B5 saved at mem[ICO].  Return = `JP` to X7 (SB7 X1 / JP B7).

## Calling convention (compiled code)
`SX7 return; JP target` (X7 = return address; EQ B0,B0,target).  Frame base B5, stack top B6,
args stored at B6+3.., function result at B6+2 (re-read after call).  Static link chain via B5.
Errors: `X7 = address of failing instruction`, jump to ASSERR 170B / INXERR 171B / DIVERR 172B / STOFLERR 174B.

## Fixed entry points the runtime must provide
GETC 101B  PUTB 102B  PUTC 103B  OPN 104B  CLS 105B  REST 106B  EOR 107B  RD 110B
WRTO 111B  TXT 112B  GETB 113B  WRTI 114B  WRTE 115B  WRTF 116B  WRTA 117B  WRTC 120B
WRTB 121B  math 132B..137B (SIN COS EXP LN SQRT ARCTAN)  errors 170B..174B.
INPUT file header (P-PTR) at 212B, OUTPUT at 233B (10-word char window buffer precedes each header).

## File header (7 words) at H; routines get B7 = H+1
H+0 P-PTR (address of the window cell; upper bits free)   H+1 spec<<55 | element size (LRL)
H+2 FET word0 (LFN left justified, zero filled)  H+3 FIRST  H+4 IN  H+5 OUT  H+6 LIMIT
spec = 2*VLEVEL+binary, VLEVEL: 0 none,1 OUT,2 PRINT,3 PUNCH,4 IN.  Standard CIO FET.

## Calls to the runtime
* WRITE(x:w): X1 = value, X2 = width (default 10, 20 for REAL/OCT), X3 = fraction digits (WRTF).
  Unwidthed CHAR is done inline (store window, B7=OUTPT+1, PUTC).  EOL (char 0) = end of line.
* READ(x): X1 = address of variable, B7 = 0 integer / 1 real, jump RD.
* GET/PUT/RESET/WEOR(F): B7 = H+1.  OPN/CLS at block entry/exit (B7 = H+1).
* TITLE('...'): X1 = address of packed text words following the jump, jump TXT.
* Math: argument at B6+3 (real), result at B6+2.

## Host side
`host/build.sh` builds the FPC host compiler; `pascal_host` emits `pasclgo.out` (octal words),
`pmd.out`, `parmlist.out` (ICO, LPJMPTAB, LOADPT, LIMCODE, ENTRYPT, ERRFLAG).
`cdcdis.py` disassembles the words.

## KRONOS 2.1 findings (DtCyber)
* Start/stop: `DISPLAY=:99 setsid sh -c 'tail -f /dev/null | node host/kronos/kd.js' &` (needs `Xvfb :99 &`; DtCyber
  dies if its stdin or operator connection closes).  Submit: `host/kronos/kj job deck`, stop: `kj shutdown`.
  Wait ~60 s for "KRONOS ready" in /tmp/claude-1000/kd.log first.
* Job deck: control cards, then `~eor` separated records.  `$COMPASS.` assembles record 2; `$LOAD,LGO.` `$EXECUTE.`
  works (source needs `END TESTNAME` for a transfer address).  `*** NAME COMPLETE/FAILED` lines end the job for kj.
* System macros exist by default (SYSCOM B1, MESSAGE, READ, WRITE, READC, WRITEC, ENDRUN, FILEB).  They expand to
  library calls: READC = `SB6 buf; SB7 maxwords; SX2 fet; RJ RDC=`, WRITEC = `SB6 buf; SX2 fet; RJ WTC=`,
  READ/WRITE = `SX2 fet; SX7 fn; RJ CIO=`, ENDRUN = `SX6 4RENDP/16D; LX6 40D; RJ SYS=`.  `LIST G,X` shows expansions.
* Hello in COMPASS works (host/kronos/t4.job).  Do NOT run `*EDIT` on OPL404 (compiles whole library, never finished).
* Kernel plan: a relocatable COMPASS program loaded at RA+100B, BSS reserving everything up to the end of the Pascal
  image so library routines land above it; vectors `EQ B0,B0,target` + NO NO at the fixed addresses; deck records:
  [control][kernel src][image as octal text, 4 words/card -> `COPYBR,INPUT,IMAGE`][Pascal source = program INPUT].
* Char mapping needed in GETC/PUTC: KRONOS display 00 (':')<->63B, 70B(')<->60B, 76B(^)<->70B.
* Open: exact READC/EOF status bits (experiment t5.job was just submitted, result unknown).

## Self-hosting (milestone reached, on the C model and on the RTL)
There is no C runtime anywhere: the entry points of the ABI above are CDC machine code (`mkrt.py` writes `runtime.asm`,
`cdcasm.py` assembles it to `runtime.img`; vectors at 100B..174B, routines from 1000B, INPUT/OUTPUT headers at 200B..).
The machine itself only has five memory-mapped words: 020B IO_RX (load: next input char, all ones at end of input),
021B IO_TX, 022B IO_FILE (name of a binary file), 023B IO_FWORD (a word of it), 024B IO_HALT.  Boot block: words 1, 2, 3 =
entry point, B5, B4 (`pasclgo2img.py` writes them; `--compiler` also stores 2048 in PARMLIST.ICO).

* `host/cdcemu.c` - reference model (pure CPU + the I/O words).  `host/selfhost.sh`: stage1 (FPC build of the unmodified
  1972 source) -> stage2 (stage1 image on cdcemu compiling pascal.cdc) -> stage3; code stream, PARMLIST and listing are
  bit-identical (34.8M instructions per compile, runtime included).
* `rtl/cdc6000_core.sv` - SystemVerilog core in the style of `~/bytecode/ocaml4142_vm_rtl.sv` (copy in `rtl/template/`):
  one FSM, synchronous memory, load port, strobe outputs.  `rtl/selfhost_rtl.sh` repeats the chain on the Verilated core:
  identical output, identical instruction count, 73.8M cycles (CPI 2.1), ~17 s per compile in simulation.
* `tests/fptest.py`: 300 random operand pairs x 28 integer/shift/float instructions; core and model agree on every word.
* `ocr/pascal.pas` is untouched.

Semantics that mattered: ones-complement adds give +0 for x+(-x) (only -0 + -0 = -0); B-register compares treat -0 == +0;
DX* on integers (exponent field 0) yields the low 48-bit integer product; the compiler derives the literals 1.0 and 10.0
as 10*0.1 and 100*0.1 with rounding multiplies, so the host's real-to-CDC conversion rounds to nearest.
Not modelled (the compiler never needs them): DX+/DX- are plain FX+/FX-, infinite/indefinite operands, real I/O
(WRTE/WRTF/RD of reals) and the math routines 132B..137B stop with a message.

## Files over NFS (nfs/, fpga/vc707-eth/)
The board has no file system and no file protocol on the UART.  The runtime built with `mkrt.py --nfs` turns GETC / PUTC / PUTB / CLS
(and the WRT* output, through `OUTCH`, an `RJ` subroutine that preserves all registers) into requests to a Pascal program,
`nfs/nfsdrv.pas`, an NFSv3-over-UDP client compiled by the unmodified 1972 compiler at ICO 40959 so that its first global
variables lie on the Ethernet device window (120000B..).  See `nfs/README.md`.
* `nfs/selfhost_nfs.sh emu|rtl`: self-compile with all files on a (modelled) NFS server; identical to the reference.
* `fpga/vc707-eth/`: board top (SGMII Ethernet from `fpga/eth-rtl`, copied from ~/bytecode), `cdc_eth_core.v` (core + packet RAM +
  ethmin registers on the core's device bus), `build.tcl` (Vivado), `mkimage.sh` (mem.hex), `sim/run.sh` (board sans PHY).
* `fpga/vc707/` is the earlier UART variant (host streams the source, credits against overflow); it still builds but is not the path.
* Constraint: the compiler's ADDRESS type is 16 bits, so a compiled program (and the driver) must lie below 65536 words, and the
  program's stack limit (boot word 3) is kept below the device window.
