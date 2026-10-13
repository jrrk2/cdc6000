# File access over NFS

The CDC core has no disk and no UART protocol for files.  The compiler's files (INPUT, OUTPUT, PASCLGO, PMDFILE) live on an
NFS server, reached over UDP through the board's Ethernet.  Nothing of this is in the compiler: `ocr/pascal.pas` is untouched.

    compiler (CDC machine code)  ->  runtime GETC/PUTC/PUTB/CLS (host/mkrt.py --nfs)  ->  mailbox  ->  nfsdrv.pas  ->  Ethernet window

* `nfslib.pas`, `shell.pas`, `nfsmain.pas` -- joined by `mkdrv.py` into `build/nfsdrv.pas`: the NFS client, **written in the 1972 Pascal** and compiled by the 1972 compiler, as a separate program
  image (ICO 40959).  It does ARP, IPv4/UDP, ONC RPC (AUTH_UNIX), portmap, mount, and NFSv3 LOOKUP / READ / CREATE /
  WRITE, one blocking request at a time, retrying four times at 1.5 s.  The wire formats are those of the OCaml client in
  `~/bytecode/io/nfs.ml`, which is known to work against a Linux server.
* The driver's first global variables **are** the device: compiled with ICO = DEVBASE-1 (DEVBASE = 120000B), `RXW`, `TXW`
  and `REG` lie exactly on the Ethernet window (a byte per 60 bit word, the ethmin I/O space), so plain array accesses
  are the hardware accesses.  Nothing in the compiler or the language had to change.
* The main body of the driver is the request handler: the run time fills the mailbox at 130020B (op, file name = the LFN
  word, argument), saves B4-B6, points them at a private stack, jumps to the driver's entry point and restores them on return
  (`CALLDRV` in `runtime_nfs.asm`).  Driver globals keep the state (handles, offsets, buffers) between requests.
* Files are named by their lower-case LFN in the export root: INPUT is `input`, OUTPUT `output`, PASCLGO `pasclgo`,
  PMDFILE `pmdfile`.  Input is the 1972 character set as UTF-8 text (the glyphs of `host/pascal.cdc`), converted to display
  code by the driver; output characters go back as ASCII; binary words as 20 octal digits and a line end.
* Addresses, the server (192.168.1.109 unless DIP switches 0-6 give a host number; export /srv/nfs/vc707/cdc) and the board (192.168.1.77, 02:00:00:43:44:43) are
  constants at the top of `nfslib.pas`; no DHCP.

## Testing
* `netmodel.h` -- the Ethernet device and a protocol-level server (ARP, portmap, mountd, nfsd on a host directory) shared by
  the C model (`host/cdcemu.c`) and the Verilator benches.  `selfhost_nfs.sh [emu|rtl]` runs the compiler on itself with
  every file on the "server": identical code and listing to the reference on both (212,398,522 instructions).
* `fpga/vc707-eth/sim/run.sh` runs the board without its PHY (core, DMA, packet RAM, registers) against the same server
  model driven at MAC-stream level.

## Limits
Every character is one driver call (about 6x the instructions of the UART version); the mock server is not a real server
(see `~/bytecode/docs/NFS_README.md` for what a Linux export needs: UDP enabled, `insecure` not needed because the source
port is 1010).  Untested on a board.


## The shell

With `host/mkrt.py --shell` the run time boots into a command shell on the UART (115200 8N1; `fpga/vc707-eth/mkimage_shell.sh`
builds the board image).  The shell is part of the driver program (`shell.pas`), so everything it does is 1972 Pascal on the
core: `dir`, `more`/`cat`, `cd`, `pwd`, `copy`, `ren`, `del`, `mkdir`, `rmdir`, `stat`, `df`, `info`, `net`, `halt`, and
`compile [name]`, which compiles `name.pas` to `name.lst`, `name.cdc`, `name.pmd` (without a name: `input` to `output`,
`pasclgo`, `pmdfile`) and then reports the number of error lines in the listing.

* Paths are relative to the current directory; `/` is the export.  Output from the compiler stays in the current directory.
* The first `compile` runs the compiler as loaded.  Every later one clears the compiler's memory and reads its image again
  from the file `pascimg` in the export root (`mkimage_shell.sh` writes it next to `mem.hex`; copy it to the export).
* Messages are `SAY('text')` in `shell.pas`; `mkdrv.py` puts the text in a table because the dialect has strings of ten
  characters only.  The UART registers are `REG[5]` (send a byte) and `REG[8]` (256 + byte, or 0 if none; a 64 byte FIFO).
* Testing: `host/cdcemu`, the Verilated core and `fpga/vc707-eth/sim/run.sh` take `-u file` for the keystrokes; a typed
  byte is only offered after the program has asked a few hundred times in the C models, as a person types slowly.


## DHCP

At start-up (and for the shell's `net` command) the driver asks for an address (DISCOVER, OFFER, REQUEST, ACK; host name
`cdc6000`) and waits three seconds for each answer, five times; if nobody answers it keeps the static address of the VALUE
section of `nfslib.pas` (192.168.1.77).  **DIP switch 7 up** skips DHCP and uses the static address.  The server is not taken from
DHCP: its host number is the value of **DIP switches 0-6** (0: the default of the VALUE section, 109) on the network of the
address the board was given (the first three bytes of it).  `info` in the shell says which it is.  The lease is not renewed (a run
is short).  The model (`netmodel.h`) answers DHCP with 192.168.1.178; `-D n` gives the DIP switches and `-N` (before
another option) takes the DHCP server away, in `host/cdcemu`, the Verilated core and the board-level simulation.


## run

`run` starts the program `compile` has just made: the run time clears the user memory (2049 up to the stack limit), reads `name.cdc`
to the load address of the compile (kept from the compiler's PARMLIST), sets up INPUT/OUTPUT as the keyboard and the screen, and enters
it.  `run n` also stores n in the cell ICO+1 (word 2049) before the start.  That cell is the compiler's PARMLIST.ICO, the origin of the
programs it makes, and `pasclgo2img.py --compiler` sets it to 2048 in the image of the compiler; a compiler that was itself made by
`compile pascal` needs `run 2048` for the same.  For any other program word 2049 is just its first variable, so it is not touched.
(Without clearing, and without n, the cell kept a stale 2048 from earlier activity, and the listing began at 004001 by accident.)
