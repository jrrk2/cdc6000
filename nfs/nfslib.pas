(* NFSDRV -- file access for the CDC 6000 core over NFS version 3 on UDP.

   A separate program, compiled by the 1972 compiler like any other and kept apart from it.  It is entered once per
   request by the run time (host/mkrt.py): the main body below is the request handler, the global variables are its
   state between requests.

   The first global variables are the device: compiled with ICO = DEVBASE-1-38912 they lie exactly on the Ethernet
   window of the core (a byte per word, the ethmin I/O space: see fpga/vc707-ethmin/ethmin_vm_core.v), so reading
   or writing them is reading or writing the hardware.  Do not insert anything above them.

     RXW  received frame            REG[0] status: 1 frame waiting, 2 transmitter busy, 4 frame truncated
     TXW  frame to send             REG[2] length of the received frame; write: release it
                                    REG[3] write: send the TXW frame, this long
                                    REG[6] milliseconds since reset
                                    REG[9] DIP switches 0-6: the host number of the server (0: keep SRVIP as compiled)

   Request (MB, filled in by the run time):
     OP 1 GETC   NAME file                -> RES display code of the next character, -1 at end of file
        OP 2 PUTC   NAME file, ARG char   -> RES 0
        OP 3 PUTW   NAME file, ARGW word  -> RES 0   (a word of binary output as 20 octal digits and a line end)
        OP 4 CLOSE  NAME file             -> RES 0   (flush the output)
     RES -2: the network or the server failed (the reason is written to the console register).
   A file is named by its logical file name (LFN) as the run time knows it: INPUT is the file "input" in the export. *)

CONST MYPORT = 1010; PMAPPORT = 111; CHUNK = 1024;
   RPCPMAP = 100000; RPCMOUNT = 100005; RPCNFS = 100003;
   P16 = 65536; P24 = 16777216;

TYPE BYTES = ARRAY [0..63] OF INTEGER;  ARRAY4 = ARRAY [0..3] OF INTEGER;

VAR MEM : ARRAY [0..38911] OF INTEGER;     (* absolute memory 2048 .. 40959, the compiler's: first, so that RXW is at 40960 with ICO 2047 *)
   RXW : ARRAY [0..2047] OF INTEGER;       (* the device: keep MEM, RXW, TXW, REG and MB first and in this order *)
   TXW : ARRAY [0..2047] OF INTEGER;
   REG : ARRAY [0..15] OF INTEGER;
   MB : RECORD OP : INTEGER; NAME : ALFA; ARGW : ALFA; ARG : INTEGER; RES : INTEGER;
      PL : ARRAY [0..15] OF INTEGER END;   (* PL: the compiler's PARMLIST after a compile, left by the run time *)

   XP, RP, XID, SPIN : INTEGER;            (* cursors into TXW and RXW, the call number, polls while waiting for the first reply *)
   INITED, NETOK : BOOLEAN;
   SRVMAC : ARRAY [0..5] OF INTEGER;
   MNTPORT, NFSPORT : INTEGER;
   FHROOT : BYTES;  FHROOTL : INTEGER;
   FROMASC : ARRAY [0..255] OF INTEGER;
   TOASC : ARRAY [0..63] OF INTEGER;
   CS : INTEGER;                           (* the current file slot *)
   SNAME : ARRAY [0..3] OF ALFA;
   SKIND : ARRAY [0..3] OF INTEGER;        (* 0 free, 1 reading, 2 writing *)
   SFH : ARRAY [0..255] OF INTEGER;        (* a handle of 64 bytes per slot *)
   SFHL : ARRAY [0..3] OF INTEGER;
   SOFF, SN, SP : ARRAY [0..3] OF INTEGER;
   SEOF : ARRAY [0..3] OF BOOLEAN;
   SBUF : ARRAY [0..4095] OF INTEGER;      (* 1024 bytes per slot *)
   NM : ARRAY [0..63] OF INTEGER;          (* the current file's name, as characters *)
   NM2 : ARRAY [0..63] OF INTEGER;  NM2N : INTEGER;       (* a second name (copy, ren) *)
   DIRFH : BYTES;  DIRL : INTEGER;         (* the directory that LOOKUP, CREATE, REMOVE ... work in *)
   CWDFH : BYTES;  CWDL : INTEGER;         (* the shell's current directory *)
   CWDP : ARRAY [0..127] OF INTEGER;  CWDPN : INTEGER;     (* and its path, for pwd *)
   FDIR : BYTES;  FDL : INTEGER;           (* the directory of the first name of ren *)
   BASE, LBASE : ARRAY [0..47] OF INTEGER;  BASEN, LBASEN : INTEGER;   (* LBASE: the base of the last compile, for run *)      (* compile: the source's base name (none: input, output ...) *)
   PIMG, NINPUT, NOUTPUT, NPASCLGO, NPMDFILE : ALFA;       (* logical file names the run time uses *)
   LINE : ARRAY [0..127] OF INTEGER;  LN : INTEGER;        (* the shell's command line *)
   TS, TL : ARRAY [0..3] OF INTEGER;  NT : INTEGER;        (* its words: start, length *)
   COOKH, COOKL : INTEGER;  VERF : ARRAY [0..1] OF INTEGER;    (* READDIRPLUS state *)
   CMDS : ARRAY [0..31] OF ALFA;  CMDW : ALFA;  LEAVE : INTEGER;
   LINES, PAGED, QUIT, LASTCR, TSTART, NCOMP, AFTERC, RESOK, CONSOLE, RUNBASE : INTEGER;
   EDGS, EDGE, EDN, EDTOP, EDTOPL, EDCUR, EDDCOL, EDHSCR, EDMOD, EDQUIT, EDMSG, EDITED : INTEGER;   (* the editor *)   (* shell state *)
   (*TXT*)
   NMN : INTEGER;
   CHW : ARRAY [1..10] OF CHAR;
   K : INTEGER;

   MYMAC : ARRAY [0..5] OF INTEGER;
   MYIP, SRVIP : ARRAY4;
   YI, SIDB, DXB, MASK, GATE : ARRAY4;  DHST, LEASE : INTEGER;     (* DHCP: address offered, server, transaction, mask, router; 0 not tried 1 bound 2 failed *)
   EXPN : INTEGER;
   EXPORT : ARRAY [0..31] OF INTEGER;
VALUE (*TXTV*)
   MYMAC = (2, 0, 0, 67, 68, 67);
   MYIP = (192, 168, 1, 77);
   SRVIP = (192, 168, 1, 109);
   EXPN = 18;
   EXPORT = (47, 115, 114, 118, 47, 110, 102, 115, 47, 118, 99, 55, 48, 55, 47, 99, 100, 99);   (* /srv/nfs/vc707/cdc *)

(* ---------------------------------------------------------------- the console register, for diagnostics *)
PROCEDURE CONS(C : INTEGER);
BEGIN REG[5] := C END;

PROCEDURE FAIL(C : INTEGER);
BEGIN CONS(78); CONS(70); CONS(83); CONS(58); CONS(C); CONS(10) END;

(* ---------------------------------------------------------------- XDR into the TX window / out of the RX window *)
PROCEDURE PUT8(V : INTEGER);
BEGIN TXW[XP] := V; XP := XP + 1 END;

PROCEDURE PUT32(V : INTEGER);
BEGIN
   TXW[XP] := V DIV P24 MOD 256;  TXW[XP+1] := V DIV P16 MOD 256;
   TXW[XP+2] := V DIV 256 MOD 256;  TXW[XP+3] := V MOD 256;
   XP := XP + 4
END;

PROCEDURE PADTO4(N : INTEGER);             (* N bytes have just been written: pad to a multiple of four *)
VAR M : INTEGER;
BEGIN
   M := N;
   WHILE M MOD 4 <> 0 DO BEGIN TXW[XP] := 0; XP := XP + 1; M := M + 1 END
END;

PROCEDURE PUTFH(VAR B : BYTES; N : INTEGER);
VAR K : INTEGER;
BEGIN
   PUT32(N);
   FOR K := 0 TO N - 1 DO PUT8(B[K]);
   PADTO4(N)
END;

FUNCTION GET8(D : INTEGER) : INTEGER;
BEGIN GET8 := RXW[RP]; RP := RP + 1 END;

FUNCTION GET32(D : INTEGER) : INTEGER;
BEGIN
   GET32 := ((RXW[RP] * 256 + RXW[RP+1]) * 256 + RXW[RP+2]) * 256 + RXW[RP+3];
   RP := RP + 4
END;

(* ---------------------------------------------------------------- frames *)
FUNCTION CSUM(FIRST, N : INTEGER) : INTEGER;   (* the Internet checksum of TXW[FIRST..FIRST+N-1] *)
VAR S, K : INTEGER;
BEGIN
   S := 0;  K := FIRST;
   WHILE K < FIRST + N DO
   BEGIN S := S + TXW[K] * 256 + TXW[K+1];  K := K + 2 END;
   WHILE S >= P16 DO S := S DIV P16 + S MOD P16;
   CSUM := 65535 - S
END;

PROCEDURE SENDFRAME(LEN : INTEGER);
BEGIN
   WHILE REG[0] DIV 2 MOD 2 = 1 DO ;
   REG[3] := LEN
END;

PROCEDURE SENDUDP(DPORT : INTEGER);            (* the payload is TXW[42..XP-1] *)
VAR LEN, K, S : INTEGER;
BEGIN
   LEN := XP;
   FOR K := 0 TO 5 DO BEGIN TXW[K] := SRVMAC[K];  TXW[6+K] := MYMAC[K] END;
   TXW[12] := 8;  TXW[13] := 0;
   TXW[14] := 69;  TXW[15] := 0;  TXW[16] := (LEN - 14) DIV 256;  TXW[17] := (LEN - 14) MOD 256;
   FOR K := 18 TO 21 DO TXW[K] := 0;
   TXW[22] := 64;  TXW[23] := 17;  TXW[24] := 0;  TXW[25] := 0;
   FOR K := 0 TO 3 DO BEGIN TXW[26+K] := MYIP[K];  TXW[30+K] := SRVIP[K] END;
   S := CSUM(14, 20);  TXW[24] := S DIV 256;  TXW[25] := S MOD 256;
   TXW[34] := MYPORT DIV 256;  TXW[35] := MYPORT MOD 256;
   TXW[36] := DPORT DIV 256;  TXW[37] := DPORT MOD 256;
   TXW[38] := (LEN - 34) DIV 256;  TXW[39] := (LEN - 34) MOD 256;
   TXW[40] := 0;  TXW[41] := 0;
   SENDFRAME(LEN)
END;

PROCEDURE RELEASE;
BEGIN REG[2] := 0 END;

(* an ARP request for our address: answer it, and release the frame *)
PROCEDURE ARPREPLY;
VAR K : INTEGER;
BEGIN
   IF (RXW[12] = 8) AND (RXW[13] = 6) AND (RXW[21] = 1) AND (RXW[38] = MYIP[0]) AND (RXW[39] = MYIP[1])
      AND (RXW[40] = MYIP[2]) AND (RXW[41] = MYIP[3]) THEN
   BEGIN
      FOR K := 0 TO 5 DO BEGIN TXW[K] := RXW[22+K];  TXW[6+K] := MYMAC[K] END;
      TXW[12] := 8;  TXW[13] := 6;  TXW[14] := 0;  TXW[15] := 1;  TXW[16] := 8;  TXW[17] := 0;
      TXW[18] := 6;  TXW[19] := 4;  TXW[20] := 0;  TXW[21] := 2;
      FOR K := 0 TO 5 DO TXW[22+K] := MYMAC[K];
      FOR K := 0 TO 3 DO TXW[28+K] := MYIP[K];
      FOR K := 0 TO 5 DO TXW[32+K] := RXW[22+K];
      FOR K := 0 TO 3 DO TXW[38+K] := RXW[28+K];
      SENDFRAME(42)
   END
END;

(* ---------------------------------------------------------------- ARP for the server *)
FUNCTION ARPSERVER(D : INTEGER) : BOOLEAN;
VAR TRIES, DEADLINE, K : INTEGER;  GOT : BOOLEAN;
BEGIN
   GOT := FALSE;  TRIES := 0;
   REPEAT
      FOR K := 0 TO 5 DO BEGIN TXW[K] := 255;  TXW[6+K] := MYMAC[K] END;
      TXW[12] := 8;  TXW[13] := 6;  TXW[14] := 0;  TXW[15] := 1;  TXW[16] := 8;  TXW[17] := 0;
      TXW[18] := 6;  TXW[19] := 4;  TXW[20] := 0;  TXW[21] := 1;
      FOR K := 0 TO 5 DO BEGIN TXW[22+K] := MYMAC[K];  TXW[32+K] := 0 END;
      FOR K := 0 TO 3 DO BEGIN TXW[28+K] := MYIP[K];  TXW[38+K] := SRVIP[K] END;
      SENDFRAME(42);
      DEADLINE := REG[6] + 1500;
      REPEAT
         IF REG[0] MOD 2 = 1 THEN
         BEGIN
            IF (RXW[12] = 8) AND (RXW[13] = 6) AND (RXW[21] = 2) AND (RXW[28] = SRVIP[0]) AND (RXW[29] = SRVIP[1])
               AND (RXW[30] = SRVIP[2]) AND (RXW[31] = SRVIP[3]) THEN
            BEGIN
               FOR K := 0 TO 5 DO SRVMAC[K] := RXW[22+K];
               GOT := TRUE
            END ELSE ARPREPLY;
            RELEASE
         END;
         SPIN := SPIN + 1
      UNTIL GOT OR (REG[6] > DEADLINE);
      TRIES := TRIES + 1
   UNTIL GOT OR (TRIES > 4);
   ARPSERVER := GOT
END;

(* ---------------------------------------------------------------- ONC RPC *)
PROCEDURE RPCCALL(PROG, VERS, PROC : INTEGER);
BEGIN
   XID := (XID + 1) MOD P24;
   XP := 42;
   PUT32(XID);  PUT32(0);  PUT32(2);  PUT32(PROG);  PUT32(VERS);  PUT32(PROC);
   PUT32(1);  PUT32(20);  PUT32(0);  PUT32(0);  PUT32(0);  PUT32(0);  PUT32(0);      (* AUTH_UNIX, uid 0, gid 0 *)
   PUT32(0);  PUT32(0)                                                               (* AUTH_NONE verifier *)
END;

(* the reply in the RX window is ours, accepted, and the call succeeded: RP is left on the result *)
FUNCTION REPLYOK(LEN : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;  V : INTEGER;
BEGIN
   OK := FALSE;
   IF (LEN >= 66) AND (RXW[12] = 8) AND (RXW[13] = 0) AND (RXW[23] = 17) AND (RXW[36] = MYPORT DIV 256)
      AND (RXW[37] = MYPORT MOD 256) THEN
   BEGIN
      RP := 42;
      V := GET32(0);                                    (* the call number *)
      IF V = XID THEN
         IF GET32(0) = 1 THEN                           (* a reply *)
            IF GET32(0) = 0 THEN                        (* accepted *)
            BEGIN
               V := GET32(0);  V := GET32(0);              (* verifier flavour, length *)
               RP := RP + (V + 3) DIV 4 * 4;
               OK := GET32(0) = 0                       (* accept_stat = SUCCESS *)
            END
   END;
   REPLYOK := OK
END;

(* send the call built in TXW and wait for its reply, resending four times; the reply stays in the RX window *)
FUNCTION TRANSACT(DPORT : INTEGER) : BOOLEAN;
VAR TRIES, DEADLINE : INTEGER;  GOT : BOOLEAN;
BEGIN
   GOT := FALSE;  TRIES := 0;
   REPEAT
      SENDUDP(DPORT);
      DEADLINE := REG[6] + 1500;
      REPEAT
         IF REG[0] MOD 2 = 1 THEN
         BEGIN
            IF REPLYOK(REG[2]) THEN GOT := TRUE ELSE BEGIN ARPREPLY;  RELEASE END
         END
      UNTIL GOT OR (REG[6] > DEADLINE);
      TRIES := TRIES + 1
   UNTIL GOT OR (TRIES > 4);
   TRANSACT := GOT
END;

(* ---------------------------------------------------------------- the mount and NFS procedures *)
FUNCTION GETPORT(PROG : INTEGER) : INTEGER;
BEGIN
   RPCCALL(RPCPMAP, 2, 3);
   PUT32(PROG);  PUT32(3);  PUT32(17);  PUT32(0);
   IF TRANSACT(PMAPPORT) THEN BEGIN GETPORT := GET32(0);  RELEASE END
   ELSE GETPORT := 0
END;

PROCEDURE TAKEFH(VAR B : BYTES; VAR N : INTEGER);   (* a file handle from the reply *)
VAR K : INTEGER;
BEGIN
   N := GET32(0);
   IF N > 64 THEN N := 0
   ELSE BEGIN
      FOR K := 0 TO N - 1 DO B[K] := RXW[RP+K];
      RP := RP + (N + 3) DIV 4 * 4
   END
END;

FUNCTION MOUNT(D : INTEGER) : BOOLEAN;
VAR K : INTEGER;  OK : BOOLEAN;
BEGIN
   OK := FALSE;
   RPCCALL(RPCMOUNT, 3, 1);
   PUT32(EXPN);  FOR K := 0 TO EXPN - 1 DO PUT8(EXPORT[K]);  PADTO4(EXPN);
   IF TRANSACT(MNTPORT) THEN
   BEGIN
      IF GET32(0) = 0 THEN BEGIN TAKEFH(FHROOT, FHROOTL);  OK := FHROOTL > 0 END;
      RELEASE
   END;
   MOUNT := OK
END;

PROCEDURE PUTNAME;                           (* the current file name as an XDR string *)
VAR K : INTEGER;
BEGIN
   PUT32(NMN);  FOR K := 0 TO NMN - 1 DO PUT8(NM[K]);  PADTO4(NMN)
END;

PROCEDURE LOADFH(VAR B : BYTES; S : INTEGER);   (* slot S's handle into B *)
VAR K : INTEGER;
BEGIN FOR K := 0 TO SFHL[S] - 1 DO B[K] := SFH[S*64+K] END;

PROCEDURE SAVEFH(S : INTEGER; VAR B : BYTES; N : INTEGER);
VAR K : INTEGER;
BEGIN FOR K := 0 TO N - 1 DO SFH[S*64+K] := B[K];  SFHL[S] := N END;

FUNCTION LOOKUPFH(VAR F : BYTES; VAR N : INTEGER) : BOOLEAN;     (* NM in DIRFH: its handle *)
VAR OK : BOOLEAN;
BEGIN
   OK := FALSE;
   RPCCALL(RPCNFS, 3, 3);
   PUTFH(DIRFH, DIRL);  PUTNAME;
   IF TRANSACT(NFSPORT) THEN
   BEGIN
      IF GET32(0) = 0 THEN BEGIN TAKEFH(F, N);  OK := N > 0 END;
      RELEASE
   END;
   LOOKUPFH := OK
END;

FUNCTION LOOKUP(S : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;  F : BYTES;  N : INTEGER;
BEGIN
   OK := LOOKUPFH(F, N);
   IF OK THEN SAVEFH(S, F, N);
   LOOKUP := OK
END;

FUNCTION CREATE(S : INTEGER) : BOOLEAN;      (* UNCHECKED: an existing file is truncated *)
VAR OK : BOOLEAN;  F : BYTES;  N : INTEGER;
BEGIN
   OK := FALSE;
   RPCCALL(RPCNFS, 3, 8);
   PUTFH(DIRFH, DIRL);  PUTNAME;
   PUT32(0);
   PUT32(1);  PUT32(420);                     (* mode 0644 *)
   PUT32(0);  PUT32(0);                       (* uid, gid *)
   PUT32(1);  PUT32(0);  PUT32(0);            (* size 0 *)
   PUT32(0);  PUT32(0);                       (* times *)
   IF TRANSACT(NFSPORT) THEN
   BEGIN
      IF GET32(0) = 0 THEN
         IF GET32(0) <> 0 THEN BEGIN TAKEFH(F, N);  IF N > 0 THEN BEGIN SAVEFH(S, F, N);  OK := TRUE END END;
      RELEASE
   END;
   CREATE := OK
END;

PROCEDURE SKIPATTR;
BEGIN IF GET32(0) = 1 THEN RP := RP + 84 END;

(* read the next piece of slot S into its buffer *)
FUNCTION READPIECE(S : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;  F : BYTES;  N, K, EOFF : INTEGER;
BEGIN
   OK := FALSE;
   RPCCALL(RPCNFS, 3, 6);
   LOADFH(F, S);  PUTFH(F, SFHL[S]);
   PUT32(0);  PUT32(SOFF[S]);  PUT32(CHUNK);
   IF TRANSACT(NFSPORT) THEN
   BEGIN
      IF GET32(0) = 0 THEN
      BEGIN
         SKIPATTR;
         K := GET32(0);                         (* count *)
         EOFF := GET32(0);
         N := GET32(0);
         IF N <= CHUNK THEN
         BEGIN
            FOR K := 0 TO N - 1 DO SBUF[S*CHUNK+K] := RXW[RP+K];
            SN[S] := N;  SP[S] := 0;  SOFF[S] := SOFF[S] + N;
            IF (EOFF <> 0) OR (N = 0) THEN SEOF[S] := TRUE;
            OK := TRUE
         END
      END;
      RELEASE
   END;
   READPIECE := OK
END;

(* write slot S's buffer *)
FUNCTION WRITEPIECE(S : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;  F : BYTES;  K, N : INTEGER;
BEGIN
   OK := TRUE;
   IF SN[S] > 0 THEN
   BEGIN
      OK := FALSE;
      RPCCALL(RPCNFS, 3, 7);
      LOADFH(F, S);  PUTFH(F, SFHL[S]);
      PUT32(0);  PUT32(SOFF[S]);  PUT32(SN[S]);  PUT32(2);
      PUT32(SN[S]);
      FOR K := 0 TO SN[S] - 1 DO PUT8(SBUF[S*CHUNK+K]);
      PADTO4(SN[S]);
      IF TRANSACT(NFSPORT) THEN
      BEGIN
         IF GET32(0) = 0 THEN
         BEGIN
            IF GET32(0) = 1 THEN RP := RP + 24;       (* wcc_data: pre_op_attr, then post_op_attr *)
            SKIPATTR;
            N := GET32(0);
            IF N = SN[S] THEN BEGIN SOFF[S] := SOFF[S] + N;  SN[S] := 0;  OK := TRUE END
         END;
         RELEASE
      END
   END;
   WRITEPIECE := OK
END;

(* let go of every file: a file that was written is flushed first *)
PROCEDURE CLOSEALL;
VAR S : INTEGER;  OK : BOOLEAN;
BEGIN
   FOR S := 0 TO 3 DO
   BEGIN
      IF SKIND[S] = 2 THEN OK := WRITEPIECE(S);
      SKIND[S] := 0
   END
END;

(* ---------------------------------------------------------------- start up *)
PROCEDURE LFNFIX(VAR A : ALFA);                  (* a logical file name as the run time makes it: unused places 0, not blank *)
VAR K : INTEGER;
BEGIN
   UNPACK(A, CHW, 1);
   FOR K := 1 TO 10 DO IF INT(CHW[K]) = 45 THEN CHW[K] := CHR(0);
   PACK(CHW, 1, A)
END;

PROCEDURE INITTABLES;
VAR K : INTEGER;
BEGIN
   FOR K := 0 TO 255 DO FROMASC[K] := 45;
   FOR K := 0 TO 25 DO BEGIN FROMASC[65+K] := K + 1;  FROMASC[97+K] := K + 1 END;
   FOR K := 0 TO 9 DO FROMASC[48+K] := 27 + K;
   FROMASC[43] := 37;  FROMASC[45] := 38;  FROMASC[42] := 39;  FROMASC[47] := 40;
   FROMASC[40] := 41;  FROMASC[41] := 42;  FROMASC[36] := 43;  FROMASC[61] := 44;
   FROMASC[32] := 45;  FROMASC[44] := 46;  FROMASC[46] := 47;  FROMASC[91] := 49;
   FROMASC[93] := 50;  FROMASC[58] := 51;  FROMASC[60] := 58;  FROMASC[62] := 59;
   FROMASC[59] := 63;  FROMASC[39] := 48;  FROMASC[94] := 56;
   FOR K := 0 TO 63 DO TOASC[K] := 63;
   FOR K := 0 TO 25 DO TOASC[K+1] := 65 + K;
   FOR K := 0 TO 9 DO TOASC[27+K] := 48 + K;
   TOASC[0] := 10;  TOASC[37] := 43;  TOASC[38] := 45;  TOASC[39] := 42;  TOASC[40] := 47;
   TOASC[41] := 40;  TOASC[42] := 41;  TOASC[43] := 36;  TOASC[44] := 61;  TOASC[45] := 32;
   TOASC[46] := 44;  TOASC[47] := 46;  TOASC[48] := 39;  TOASC[49] := 91;  TOASC[50] := 93;
   TOASC[51] := 58;  TOASC[52] := 35;  TOASC[53] := 64;  TOASC[54] := 124;  TOASC[55] := 38;
   TOASC[56] := 94;  TOASC[57] := 33;  TOASC[58] := 60;  TOASC[59] := 62;  TOASC[60] := 123;
   TOASC[61] := 125;  TOASC[62] := 126;  TOASC[63] := 59;
   FOR K := 0 TO 3 DO BEGIN SKIND[K] := 0;  SFHL[K] := 0;  SOFF[K] := 0;  SN[K] := 0;  SP[K] := 0;  SEOF[K] := FALSE END;
   PIMG := 'PASCIMG   ';  NINPUT := 'INPUT     ';  NOUTPUT := 'OUTPUT    ';  NPASCLGO := 'PASCLGO   ';  NPMDFILE := 'PMDFILE   ';
   LFNFIX(PIMG);  LFNFIX(NINPUT);  LFNFIX(NOUTPUT);  LFNFIX(NPASCLGO);  LFNFIX(NPMDFILE);
   BASEN := 0;  CWDL := 0;  DIRL := 0;  CWDPN := 0
END;

(* ---------------------------------------------------------------- DHCP (RFC 2131): the address of the board *)
PROCEDURE DHPUT(C, L : INTEGER);               (* an option: code and length; the data follow at TXW[XP] *)
BEGIN TXW[XP] := C;  TXW[XP+1] := L;  XP := XP + 2 END;

PROCEDURE DHSEND(MT : INTEGER);                (* a broadcast: MT 1 DISCOVER, 3 REQUEST (for YI from the server SIDB) *)
VAR K, N : INTEGER;
BEGIN
   FOR K := 14 TO 349 DO TXW[K] := 0;
   FOR K := 0 TO 5 DO BEGIN TXW[K] := 255;  TXW[6+K] := MYMAC[K] END;
   TXW[12] := 8;  TXW[13] := 0;
   TXW[42] := 1;  TXW[43] := 1;  TXW[44] := 6;                            (* BOOTREQUEST, Ethernet, 6 bytes *)
   FOR K := 0 TO 3 DO TXW[46+K] := DXB[K];
   TXW[52] := 128;                                                        (* flags: answer by broadcast *)
   FOR K := 0 TO 5 DO TXW[70+K] := MYMAC[K];
   TXW[278] := 99;  TXW[279] := 130;  TXW[280] := 83;  TXW[281] := 99;     (* the magic cookie *)
   XP := 282;
   DHPUT(53, 1);  TXW[XP] := MT;  XP := XP + 1;
   DHPUT(61, 7);  TXW[XP] := 1;  FOR K := 0 TO 5 DO TXW[XP+1+K] := MYMAC[K];  XP := XP + 7;
   IF MT = 3 THEN
   BEGIN
      DHPUT(50, 4);  FOR K := 0 TO 3 DO TXW[XP+K] := YI[K];  XP := XP + 4;
      DHPUT(54, 4);  FOR K := 0 TO 3 DO TXW[XP+K] := SIDB[K];  XP := XP + 4
   END;
   DHPUT(12, 7);                                                          (* the host name cdc6000 *)
   TXW[XP] := 99;  TXW[XP+1] := 100;  TXW[XP+2] := 99;  TXW[XP+3] := 54;  TXW[XP+4] := 48;  TXW[XP+5] := 48;  TXW[XP+6] := 48;
   XP := XP + 7;
   DHPUT(55, 4);  TXW[XP] := 1;  TXW[XP+1] := 3;  TXW[XP+2] := 6;  TXW[XP+3] := 51;  XP := XP + 4;
   TXW[XP] := 255;  XP := XP + 1;
   IF XP < 300 THEN XP := 300;
   TXW[14] := 69;  TXW[16] := (XP - 14) DIV 256;  TXW[17] := (XP - 14) MOD 256;    (* IP, from 0.0.0.0 to 255.255.255.255 *)
   TXW[22] := 64;  TXW[23] := 17;
   FOR K := 30 TO 33 DO TXW[K] := 255;
   N := CSUM(14, 20);  TXW[24] := N DIV 256;  TXW[25] := N MOD 256;
   TXW[35] := 68;  TXW[37] := 67;  TXW[38] := (XP - 34) DIV 256;  TXW[39] := (XP - 34) MOD 256;    (* UDP 68 to 67 *)
   SENDFRAME(XP)
END;

(* wait up to three seconds for a DHCP reply of type WANT (2 offer, 5 acknowledge) to our transaction; it leaves the address in YI,
   the server in SIDB, the mask, the router and the lease *)
FUNCTION DHWAIT(WANT : INTEGER) : BOOLEAN;
VAR DEADLINE, K, T, C, L, O, E : INTEGER;  GOT, MINE : BOOLEAN;
BEGIN
   GOT := FALSE;  DEADLINE := REG[6] + 3000;
   REPEAT
      IF REG[0] MOD 2 = 1 THEN
      BEGIN
         E := REG[2];
         MINE := (E >= 290) AND (RXW[12] = 8) AND (RXW[13] = 0) AND (RXW[23] = 17) AND (RXW[36] = 0) AND (RXW[37] = 68)
                 AND (RXW[42] = 2) AND (RXW[278] = 99) AND (RXW[279] = 130) AND (RXW[280] = 83) AND (RXW[281] = 99);
         IF MINE THEN
            FOR K := 0 TO 3 DO IF RXW[46+K] <> DXB[K] THEN MINE := FALSE;
         IF MINE THEN
            FOR K := 0 TO 5 DO IF RXW[70+K] <> MYMAC[K] THEN MINE := FALSE;
         IF MINE THEN
         BEGIN
            T := 0;  O := 282;
            WHILE (O < E - 1) AND (RXW[O] <> 255) DO
            BEGIN
               C := RXW[O];
               IF C = 0 THEN O := O + 1
               ELSE
               BEGIN
                  L := RXW[O+1];
                  IF C = 53 THEN T := RXW[O+2]
                  ELSE IF (C = 54) AND (L = 4) THEN FOR K := 0 TO 3 DO SIDB[K] := RXW[O+2+K]
                  ELSE IF (C = 1) AND (L = 4) THEN FOR K := 0 TO 3 DO MASK[K] := RXW[O+2+K]
                  ELSE IF (C = 3) AND (L >= 4) THEN FOR K := 0 TO 3 DO GATE[K] := RXW[O+2+K]
                  ELSE IF (C = 51) AND (L = 4) THEN
                     LEASE := ((RXW[O+2] * 256 + RXW[O+3]) * 256 + RXW[O+4]) * 256 + RXW[O+5];
                  O := O + 2 + L
               END
            END;
            IF T = WANT THEN
            BEGIN
               FOR K := 0 TO 3 DO YI[K] := RXW[58+K];
               GOT := TRUE
            END
         END;
         RELEASE
      END
   UNTIL GOT OR (REG[6] > DEADLINE);
   DHWAIT := GOT
END;

FUNCTION DHCP(D : INTEGER) : BOOLEAN;           (* discover, offer, request, acknowledge; MYIP is set *)
VAR TRIES, K, V : INTEGER;  OK : BOOLEAN;
BEGIN
   OK := FALSE;  TRIES := 0;  LEASE := 0;
   FOR K := 0 TO 3 DO BEGIN MASK[K] := 0;  GATE[K] := 0 END;
   REPEAT
      V := REG[10] + REG[6] * 7919 + TRIES * 104729 + 12345;
      DXB[0] := V DIV P24 MOD 256;  DXB[1] := V DIV P16 MOD 256;  DXB[2] := V DIV 256 MOD 256;  DXB[3] := V MOD 256;
      DHSEND(1);
      IF DHWAIT(2) THEN
      BEGIN
         DHSEND(3);
         IF DHWAIT(5) THEN OK := TRUE
      END;
      TRIES := TRIES + 1
   UNTIL OK OR (TRIES >= 5);
   IF OK THEN FOR K := 0 TO 3 DO MYIP[K] := YI[K];
   DHCP := OK
END;

FUNCTION NETSTART(D : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;
BEGIN
   OK := FALSE;
   DHST := 0;
   IF REG[9] DIV 128 MOD 2 = 0 THEN               (* DIP switch 7 up: keep the address of the VALUE below *)
   BEGIN
      IF DHCP(0) THEN
      BEGIN
         DHST := 1;
         FOR K := 0 TO 2 DO SRVIP[K] := MYIP[K]      (* the server is on our network: its host number is the DIP value *)
      END
      ELSE DHST := 2
   END;
   IF ARPSERVER(0) THEN
   BEGIN
      (* The server answers a repeated CREATE or WRITE from its duplicate request cache, without doing it, if the
         call number, the arguments and our port are the same as in an earlier run.  Every boot would otherwise
         start at the same number, so it starts where the time to the first ARP reply (link up, server latency) says. *)
      XID := (XID + REG[6] * 4099 + SPIN * 131 + REG[10]) MOD P24;
      MNTPORT := GETPORT(RPCMOUNT);
      IF MNTPORT = 0 THEN FAIL(49)
      ELSE IF NOT MOUNT(0) THEN FAIL(50)
      ELSE BEGIN
         NFSPORT := GETPORT(RPCNFS);
         IF NFSPORT = 0 THEN NFSPORT := 2049;
         FOR K := 0 TO FHROOTL - 1 DO BEGIN CWDFH[K] := FHROOT[K];  DIRFH[K] := FHROOT[K] END;
         CWDL := FHROOTL;  DIRL := FHROOTL;  CWDPN := 0;
         OK := TRUE
      END
   END ELSE FAIL(48);
   NETSTART := OK
END;

(* ---------------------------------------------------------------- files *)
(* compile foo: INPUT is foo.pas, OUTPUT foo.lst, PASCLGO foo.cdc, PMDFILE foo.pmd *)
PROCEDURE MAPBASE;
VAR E, K : INTEGER;
BEGIN
   E := 0;
   IF MB.NAME = NINPUT THEN E := 1
   ELSE IF MB.NAME = NOUTPUT THEN E := 2
   ELSE IF MB.NAME = NPASCLGO THEN E := 3
   ELSE IF MB.NAME = NPMDFILE THEN E := 4;
   IF E > 0 THEN
   BEGIN
      FOR K := 0 TO BASEN - 1 DO NM[K] := BASE[K];
      NMN := BASEN;  NM[NMN] := 46;  NMN := NMN + 1;
      IF E = 1 THEN BEGIN NM[NMN] := 112;  NM[NMN+1] := 97;  NM[NMN+2] := 115 END
      ELSE IF E = 2 THEN BEGIN NM[NMN] := 108;  NM[NMN+1] := 115;  NM[NMN+2] := 116 END
      ELSE IF E = 3 THEN BEGIN NM[NMN] := 99;  NM[NMN+1] := 100;  NM[NMN+2] := 99 END
      ELSE BEGIN NM[NMN] := 112;  NM[NMN+1] := 109;  NM[NMN+2] := 100 END;
      NMN := NMN + 3
   END
END;

(* find or open the file MB.NAME; the result is its slot, or -1 *)
FUNCTION FINDSLOT(KIND : INTEGER) : INTEGER;
VAR S, F, K, C : INTEGER;
BEGIN
   F := -1;  S := 0;
   WHILE (S < 4) AND (F < 0) DO
   BEGIN IF (SKIND[S] <> 0) AND (SNAME[S] = MB.NAME) THEN F := S;  S := S + 1 END;
   IF F < 0 THEN                               (* not open: a free slot *)
   BEGIN
      S := 0;
      WHILE (S < 4) AND (F < 0) DO BEGIN IF SKIND[S] = 0 THEN F := S;  S := S + 1 END;
      IF F >= 0 THEN
      BEGIN
         UNPACK(MB.NAME, CHW, 1);               (* the logical file name, lower case, as the remote name *)
         NMN := 0;
         FOR K := 1 TO 10 DO
         BEGIN
            C := INT(CHW[K]);
            IF (C >= 1) AND (C <= 26) THEN BEGIN NM[NMN] := 96 + C;  NMN := NMN + 1 END
            ELSE IF (C >= 27) AND (C <= 36) THEN BEGIN NM[NMN] := 48 + C - 27;  NMN := NMN + 1 END
         END;
         IF MB.NAME = PIMG THEN
            FOR K := 0 TO FHROOTL - 1 DO BEGIN DIRFH[K] := FHROOT[K];  DIRL := FHROOTL END
         ELSE
         BEGIN
            FOR K := 0 TO CWDL - 1 DO DIRFH[K] := CWDFH[K];
            DIRL := CWDL;
            IF BASEN > 0 THEN MAPBASE                    (* compile NAME: input, output, pasclgo, pmdfile *)
         END;
         SNAME[F] := MB.NAME;  SKIND[F] := KIND;  SOFF[F] := 0;  SN[F] := 0;  SP[F] := 0;  SEOF[F] := FALSE;
         IF KIND = 1 THEN BEGIN IF NOT LOOKUP(F) THEN BEGIN SKIND[F] := 0;  F := -1;  FAIL(76) END END
         ELSE BEGIN IF NOT CREATE(F) THEN BEGIN SKIND[F] := 0;  F := -1;  FAIL(67) END END
      END
   END;
   FINDSLOT := F
END;

(* the next byte of slot S, or -1 *)
FUNCTION NEXTBYTE(S : INTEGER) : INTEGER;
VAR B : INTEGER;
BEGIN
   B := -1;
   IF (SP[S] >= SN[S]) AND NOT SEOF[S] THEN
      IF NOT READPIECE(S) THEN SEOF[S] := TRUE;
   IF SP[S] < SN[S] THEN BEGIN B := SBUF[S*CHUNK+SP[S]];  SP[S] := SP[S] + 1 END;
   NEXTBYTE := B
END;

(* the next character of the 1972 character set, UTF-8 glyphs included; -1 at end of file *)
FUNCTION GETCH(S : INTEGER) : INTEGER;
VAR B, B1, B2, R : INTEGER;
BEGIN
   B := NEXTBYTE(S);
   IF B < 0 THEN R := -1
   ELSE IF B = 10 THEN R := 0
   ELSE IF (B = 226) OR (B = 194) THEN
   BEGIN
      B1 := 0;
      IF B = 226 THEN B1 := NEXTBYTE(S);
      B2 := NEXTBYTE(S);
      R := 45;
      IF B = 194 THEN R := 62
      ELSE IF B1 = 136 THEN BEGIN IF B2 = 167 THEN R := 55 ELSE IF B2 = 168 THEN R := 54 END
      ELSE IF B1 = 137 THEN
      BEGIN
         IF B2 = 160 THEN R := 52 ELSE IF B2 = 164 THEN R := 60
         ELSE IF B2 = 165 THEN R := 61 ELSE IF B2 = 161 THEN R := 48
      END
      ELSE IF B1 = 134 THEN
      BEGIN IF B2 = 145 THEN R := 56 ELSE IF B2 = 147 THEN R := 57 ELSE IF B2 = 146 THEN R := 53 END
   END
   ELSE R := FROMASC[B];
   GETCH := R
END;

(* append a byte to slot S's output, writing the buffer when it is full *)
FUNCTION PUTBYTE(S, B : INTEGER) : BOOLEAN;
VAR OK : BOOLEAN;
BEGIN
   OK := TRUE;
   SBUF[S*CHUNK+SN[S]] := B;  SN[S] := SN[S] + 1;
   IF SN[S] = CHUNK THEN OK := WRITEPIECE(S);
   PUTBYTE := OK
END;

FUNCTION PUTWORD(S : INTEGER) : BOOLEAN;      (* MB.ARGW as 20 octal digits and a line end *)
VAR K : INTEGER;  OK : BOOLEAN;
BEGIN
   OK := TRUE;
   UNPACK(MB.ARGW, CHW, 1);
   FOR K := 1 TO 10 DO
   BEGIN
      IF OK THEN OK := PUTBYTE(S, 48 + INT(CHW[K]) DIV 8);
      IF OK THEN OK := PUTBYTE(S, 48 + INT(CHW[K]) MOD 8)
   END;
   IF OK THEN OK := PUTBYTE(S, 10);
   PUTWORD := OK
END;

