(* ---------------------------------------------------------------- the shell
   Commands typed on the UART (115200 8N1): dir, more, compile and the NFS utilities.  Part of the NFS driver program (nfs/mkdrv.py
   joins nfslib.pas, this file and nfsmain.pas): the 1972 Pascal has no separate compilation.  Text is written as SAY and a quoted
   string; mkdrv.py turns it into SAY with an offset into TXT.  A \n in the text is a carriage return and a line feed.

   The console register REG[5] takes a byte to send; REG[8] gives 256 + a received byte, or 0 if there is none. *)

PROCEDURE PUTCH(C : INTEGER);
BEGIN REG[5] := C END;

PROCEDURE CRLF;
BEGIN PUTCH(13);  PUTCH(10) END;

PROCEDURE SAY(P : INTEGER);                    (* the message at TXT[P], ending with 0 *)
VAR Q : INTEGER;
BEGIN
   Q := P;
   WHILE TXT[Q] <> 0 DO BEGIN PUTCH(TXT[Q]);  Q := Q + 1 END
END;

PROCEDURE SAYN(V : INTEGER);                   (* a decimal number *)
VAR D : ARRAY [0..15] OF INTEGER;  N, W : INTEGER;
BEGIN
   W := V;  N := 0;
   IF W = 0 THEN BEGIN D[0] := 48;  N := 1 END;
   WHILE W > 0 DO BEGIN D[N] := 48 + W MOD 10;  W := W DIV 10;  N := N + 1 END;
   WHILE N > 0 DO BEGIN N := N - 1;  PUTCH(D[N]) END
END;

PROCEDURE SAYR(V, WD : INTEGER);               (* right aligned in WD columns *)
VAR W, N, K : INTEGER;
BEGIN
   N := 1;  W := V DIV 10;
   WHILE W > 0 DO BEGIN N := N + 1;  W := W DIV 10 END;
   FOR K := N + 1 TO WD DO PUTCH(32);
   SAYN(V)
END;

PROCEDURE SAY2(V : INTEGER);
BEGIN PUTCH(48 + V DIV 10);  PUTCH(48 + V MOD 10) END;

PROCEDURE SAYIP(VAR A : ARRAY4);
VAR K : INTEGER;
BEGIN
   FOR K := 0 TO 3 DO BEGIN SAYN(A[K]);  IF K < 3 THEN PUTCH(46) END
END;

(* a byte from the UART; with ARPOK the network is looked after while waiting (not when a reply is held in the RX window) *)
FUNCTION GETKEY(ARPOK : INTEGER) : INTEGER;
VAR V : INTEGER;
BEGIN
   REPEAT
      V := REG[8];
      IF V = 0 THEN
         IF ARPOK = 1 THEN
            IF REG[0] MOD 2 = 1 THEN BEGIN ARPREPLY;  RELEASE END
   UNTIL V <> 0;
   GETKEY := V MOD 256
END;

FUNCTION POLLKEY(D : INTEGER) : INTEGER;       (* a byte if one has arrived, else 0 *)
VAR V : INTEGER;
BEGIN
   V := REG[8];
   POLLKEY := V MOD 256
END;

PROCEDURE GETLINE;
VAR C, DONE : INTEGER;
BEGIN
   LN := 0;  DONE := 0;
   REPEAT
      C := GETKEY(1);
      IF C = 13 THEN BEGIN DONE := 1;  LASTCR := 1 END
      ELSE IF C = 10 THEN BEGIN IF LASTCR = 0 THEN DONE := 1;  LASTCR := 0 END
      ELSE
      BEGIN
         LASTCR := 0;
         IF (C = 8) OR (C = 127) THEN
         BEGIN IF LN > 0 THEN BEGIN LN := LN - 1;  PUTCH(8);  PUTCH(32);  PUTCH(8) END END
         ELSE IF C = 3 THEN BEGIN LN := 0;  DONE := 1 END
         ELSE IF (C >= 32) AND (C < 127) AND (LN < 120) THEN BEGIN LINE[LN] := C;  LN := LN + 1;  PUTCH(C) END
      END
   UNTIL DONE = 1;
   CRLF
END;

PROCEDURE SPLIT;                               (* the line into at most four words *)
VAR P : INTEGER;
BEGIN
   NT := 0;  P := 0;
   WHILE (P < LN) AND (NT < 4) DO
   BEGIN
      WHILE (P < LN) AND (LINE[P] = 32) DO P := P + 1;
      IF P < LN THEN
      BEGIN
         TS[NT] := P;
         WHILE (P < LN) AND (LINE[P] <> 32) DO P := P + 1;
         TL[NT] := P - TS[NT];  NT := NT + 1
      END
   END
END;

PROCEDURE MAKECMD;                             (* the first word as ten characters of the 1972 set, upper case, in one word *)
VAR K, C : INTEGER;
BEGIN
   FOR K := 1 TO 10 DO
   BEGIN
      C := 45;
      IF K <= TL[0] THEN C := FROMASC[LINE[TS[0] + K - 1]];
      CHW[K] := CHR(C)
   END;
   PACK(CHW, 1, CMDW)
END;

PROCEDURE NFSERR(E : INTEGER);
BEGIN
   IF E = 2 THEN SAY('no such file or directory\n')
   ELSE IF E = 13 THEN SAY('permission denied\n')
   ELSE IF E = 17 THEN SAY('file exists\n')
   ELSE IF E = 20 THEN SAY('not a directory\n')
   ELSE IF E = 21 THEN SAY('is a directory\n')
   ELSE IF E = 66 THEN SAY('directory not empty\n')
   ELSE BEGIN SAY('nfs error ');  SAYN(E);  CRLF END
END;

(* ---------------------------------------------------------------- names and directories *)
PROCEDURE COPYFH(VAR D : BYTES; VAR DL : INTEGER; VAR S : BYTES; SL : INTEGER);
VAR K : INTEGER;
BEGIN
   FOR K := 0 TO SL - 1 DO D[K] := S[K];
   DL := SL
END;

(* the word LINE[S..S+N-1] is a path: DIRFH is left on the directory of its last name, which is in NM; RESOK 0 if a directory in it is
   missing.  A path that ends with a slash leaves NM empty. *)
PROCEDURE RESOLVE(S, N : INTEGER);
VAR P, E, K, EN, FN : INTEGER;  F : BYTES;
BEGIN
   RESOK := 1;
   IF (N > 0) AND (LINE[S] = 47) THEN COPYFH(DIRFH, DIRL, FHROOT, FHROOTL)
   ELSE COPYFH(DIRFH, DIRL, CWDFH, CWDL);
   NMN := 0;  P := S;  EN := S + N;
   WHILE (RESOK = 1) AND (P < EN) DO
   BEGIN
      E := P;
      WHILE (E < EN) AND (LINE[E] <> 47) DO E := E + 1;
      NMN := E - P;
      IF NMN > 63 THEN NMN := 63;
      FOR K := 0 TO NMN - 1 DO NM[K] := LINE[P + K];
      IF E < EN THEN
      BEGIN
         IF NMN > 0 THEN
         BEGIN
            IF LOOKUPFH(F, FN) THEN COPYFH(DIRFH, DIRL, F, FN) ELSE RESOK := 0
         END;
         NMN := 0;  P := E + 1
      END
      ELSE P := EN
   END
END;

(* after a successful cd: the path text follows the word *)
PROCEDURE APPLYPATH(S, N : INTEGER);
VAR P, E, EN, K, L : INTEGER;
BEGIN
   IF (N > 0) AND (LINE[S] = 47) THEN CWDPN := 0;
   P := S;  EN := S + N;
   WHILE P < EN DO
   BEGIN
      E := P;
      WHILE (E < EN) AND (LINE[E] <> 47) DO E := E + 1;
      L := E - P;
      IF (L = 2) AND (LINE[P] = 46) AND (LINE[P+1] = 46) THEN
      BEGIN
         WHILE (CWDPN > 0) AND (CWDP[CWDPN-1] <> 47) DO CWDPN := CWDPN - 1;
         IF CWDPN > 0 THEN CWDPN := CWDPN - 1
      END
      ELSE IF (L > 0) AND NOT ((L = 1) AND (LINE[P] = 46)) THEN
         IF CWDPN + L + 1 < 127 THEN
         BEGIN
            CWDP[CWDPN] := 47;  CWDPN := CWDPN + 1;
            FOR K := 0 TO L - 1 DO BEGIN CWDP[CWDPN] := LINE[P + K];  CWDPN := CWDPN + 1 END
         END;
      P := E + 1
   END
END;

PROCEDURE PROMPT;
VAR K : INTEGER;
BEGIN
   SAY('cdc:');
   IF CWDPN = 0 THEN PUTCH(47) ELSE FOR K := 0 TO CWDPN - 1 DO PUTCH(CWDP[K]);
   SAY('> ')
END;

PROCEDURE DOPWD;
VAR K : INTEGER;
BEGIN
   IF CWDPN = 0 THEN PUTCH(47) ELSE FOR K := 0 TO CWDPN - 1 DO PUTCH(CWDP[K]);
   CRLF
END;

PROCEDURE DOCD;
VAR F : BYTES;  FN : INTEGER;  OK : BOOLEAN;
BEGIN
   IF NT < 2 THEN BEGIN COPYFH(CWDFH, CWDL, FHROOT, FHROOTL);  CWDPN := 0 END
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      OK := RESOK = 1;
      IF OK THEN
      BEGIN
         IF NMN > 0 THEN OK := LOOKUPFH(F, FN) ELSE COPYFH(F, FN, DIRFH, DIRL)
      END;
      IF OK THEN BEGIN COPYFH(CWDFH, CWDL, F, FN);  APPLYPATH(TS[1], TL[1]) END
      ELSE SAY('no such directory\n')
   END
END;

(* ---------------------------------------------------------------- attributes, dates and the listing *)
PROCEDURE ATTR3(VAR TY, SZ, MT : INTEGER);     (* a fattr3 in the reply: type, size (low 32 bits), mtime *)
VAR HI : INTEGER;
BEGIN
   TY := GET32(0);  RP := RP + 16;
   HI := GET32(0);  SZ := GET32(0);
   RP := RP + 40;
   MT := GET32(0);
   RP := RP + 12
END;

PROCEDURE SAYDATE(T : INTEGER);                   (* seconds since 1970 as yyyy-mm-dd hh:mm, UTC *)
VAR Z, ERA, DOE, YOE, Y, DOY, MP, D, M, S : INTEGER;
BEGIN
   S := T MOD 86400;  Z := T DIV 86400 + 719468;
   ERA := Z DIV 146097;  DOE := Z - ERA * 146097;
   YOE := (DOE - DOE DIV 1460 + DOE DIV 36524 - DOE DIV 146096) DIV 365;
   Y := YOE + ERA * 400;
   DOY := DOE - (365 * YOE + YOE DIV 4 - YOE DIV 100);
   MP := (5 * DOY + 2) DIV 153;
   D := DOY - (153 * MP + 2) DIV 5 + 1;
   IF MP < 10 THEN M := MP + 3 ELSE M := MP - 9;
   IF M <= 2 THEN Y := Y + 1;
   SAYN(Y);  PUTCH(45);  SAY2(M);  PUTCH(45);  SAY2(D);  PUTCH(32);
   SAY2(S DIV 3600);  PUTCH(58);  SAY2(S MOD 3600 DIV 60)
END;

PROCEDURE ASKMORE;
VAR K, J : INTEGER;
BEGIN
   SAY('--More--');
   K := GETKEY(0);
   PUTCH(13);  FOR J := 1 TO 8 DO PUTCH(32);  PUTCH(13);
   IF (K = 113) OR (K = 81) OR (K = 3) THEN QUIT := 1
   ELSE IF (K = 13) OR (K = 10) THEN LINES := 21
   ELSE LINES := 0
END;

PROCEDURE ENDLINE;                             (* a line has been written: page *)
BEGIN
   CRLF;  LINES := LINES + 1;
   IF PAGED = 1 THEN
      IF LINES >= 22 THEN ASKMORE
END;

PROCEDURE DODIR;                               (* list the directory DIRFH *)
VAR MORE, N, NL, NS, TY, SZ, MT, K, HIDE : INTEGER;
BEGIN
   COOKH := 0;  COOKL := 0;  VERF[0] := 0;  VERF[1] := 0;  MORE := 1;  LINES := 0;  QUIT := 0;
   WHILE MORE = 1 DO
   BEGIN
      RPCCALL(RPCNFS, 3, 17);
      PUTFH(DIRFH, DIRL);
      PUT32(COOKH);  PUT32(COOKL);  PUT32(VERF[0]);  PUT32(VERF[1]);  PUT32(512);  PUT32(1400);
      MORE := 0;
      IF NOT TRANSACT(NFSPORT) THEN SAY('no reply from the server\n')
      ELSE
      BEGIN
         N := GET32(0);
         IF N <> 0 THEN NFSERR(N)
         ELSE
         BEGIN
            SKIPATTR;
            VERF[0] := GET32(0);  VERF[1] := GET32(0);
            WHILE GET32(0) = 1 DO
            BEGIN
               N := GET32(0);  N := GET32(0);                   (* the file id *)
               NL := GET32(0);  NS := RP;  RP := RP + (NL + 3) DIV 4 * 4;
               COOKH := GET32(0);  COOKL := GET32(0);
               TY := 0;  SZ := 0;  MT := 0;
               IF GET32(0) = 1 THEN ATTR3(TY, SZ, MT);
               IF GET32(0) = 1 THEN BEGIN N := GET32(0);  RP := RP + (N + 3) DIV 4 * 4 END;
               HIDE := 0;
               IF (NL = 1) AND (RXW[NS] = 46) THEN HIDE := 1;
               IF (NL = 2) AND (RXW[NS] = 46) AND (RXW[NS+1] = 46) THEN HIDE := 1;
               IF (HIDE = 0) AND (QUIT = 0) THEN
               BEGIN
                  IF TY = 2 THEN PUTCH(100) ELSE IF TY = 5 THEN PUTCH(108) ELSE PUTCH(45);
                  SAYR(SZ, 11);  PUTCH(32);  SAYDATE(MT);  PUTCH(32);
                  FOR K := 0 TO NL - 1 DO PUTCH(RXW[NS+K]);
                  ENDLINE
               END
            END;
            IF GET32(0) = 0 THEN MORE := 1;                      (* not the end of the directory *)
            IF QUIT = 1 THEN MORE := 0
         END;
         RELEASE
      END
   END
END;

PROCEDURE DODIRCMD;
VAR F : BYTES;  FN : INTEGER;  OK : BOOLEAN;
BEGIN
   PAGED := 1;
   OK := TRUE;
   IF NT < 2 THEN COPYFH(DIRFH, DIRL, CWDFH, CWDL)
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      OK := RESOK = 1;
      IF OK THEN
         IF NMN > 0 THEN BEGIN OK := LOOKUPFH(F, FN);  IF OK THEN COPYFH(DIRFH, DIRL, F, FN) END
   END;
   IF OK THEN DODIR ELSE SAY('no such directory\n')
END;

(* ---------------------------------------------------------------- more *)
PROCEDURE OPENSLOT(S : INTEGER);               (* NM in DIRFH for reading, in slot S *)
BEGIN
   SKIND[S] := 1;  SOFF[S] := 0;  SN[S] := 0;  SP[S] := 0;  SEOF[S] := FALSE
END;

PROCEDURE DOMORE(PG : INTEGER);
VAR B, K : INTEGER;
BEGIN
   IF NT < 2 THEN SAY('usage: more file\n')
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      OPENSLOT(3);
      IF (RESOK = 1) AND (NMN > 0) AND LOOKUP(3) THEN
      BEGIN
         PAGED := PG;  LINES := 0;  QUIT := 0;
         REPEAT
            B := NEXTBYTE(3);
            IF B >= 0 THEN
            BEGIN
               IF B = 10 THEN
               BEGIN
                  ENDLINE;
                  IF PG = 0 THEN
                     IF POLLKEY(0) = 3 THEN QUIT := 1       (* paged: ^C is read at the --More-- prompt *)
               END
               ELSE IF B = 9 THEN FOR K := 1 TO 8 DO PUTCH(32)
               ELSE IF B = 13 THEN B := B
               ELSE IF (B >= 32) AND (B < 127) THEN PUTCH(B)
               ELSE PUTCH(46)
            END
         UNTIL (B < 0) OR (QUIT = 1);
         CRLF
      END
      ELSE SAY('cannot open the file\n');
      SKIND[3] := 0
   END
END;

(* ---------------------------------------------------------------- the file utilities *)
PROCEDURE DOSTAT;
VAR F : BYTES;  FN, N, TY, SZ, MT, MODE : INTEGER;
BEGIN
   IF NT < 2 THEN SAY('usage: stat file\n')
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      IF (RESOK = 1) AND ((NMN = 0) OR LOOKUPFH(F, FN)) THEN
      BEGIN
         IF NMN = 0 THEN COPYFH(F, FN, DIRFH, DIRL);
         RPCCALL(RPCNFS, 3, 1);
         PUTFH(F, FN);
         IF TRANSACT(NFSPORT) THEN
         BEGIN
            N := GET32(0);
            IF N <> 0 THEN NFSERR(N)
            ELSE
            BEGIN
               MODE := RXW[RP+4+3] + RXW[RP+4+2] * 256;
               ATTR3(TY, SZ, MT);
               SAY('type ');  SAYN(TY);  SAY('  mode ');  SAYN(MODE DIV 64 MOD 8);  SAYN(MODE DIV 8 MOD 8);  SAYN(MODE MOD 8);
               SAY('  size ');  SAYN(SZ);  SAY('  modified ');  SAYDATE(MT);  CRLF
            END;
            RELEASE
         END
         ELSE SAY('no reply from the server\n')
      END
      ELSE SAY('no such file\n')
   END
END;

PROCEDURE DODF;
VAR N, HI, LO, K : INTEGER;
BEGIN
   RPCCALL(RPCNFS, 3, 18);
   PUTFH(CWDFH, CWDL);
   IF TRANSACT(NFSPORT) THEN
   BEGIN
      N := GET32(0);
      IF N <> 0 THEN NFSERR(N)
      ELSE
      BEGIN
         SKIPATTR;
         HI := GET32(0);  LO := GET32(0);
         SAY('total ');  SAYN(HI * 4096 + LO DIV 1048576);  SAY(' MB');
         HI := GET32(0);  LO := GET32(0);
         SAY(', free ');  SAYN(HI * 4096 + LO DIV 1048576);  SAY(' MB\n')
      END;
      RELEASE
   END
   ELSE SAY('no reply from the server\n')
END;

(* one NFS procedure on DIRFH and NM: REMOVE 12, RMDIR 13; MKDIR 9 gets the mode of a directory *)
PROCEDURE DOSIMPLE(PROC : INTEGER);
VAR N : INTEGER;
BEGIN
   IF NT < 2 THEN SAY('usage: command name\n')
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      IF (RESOK = 1) AND (NMN > 0) THEN
      BEGIN
         RPCCALL(RPCNFS, 3, PROC);
         PUTFH(DIRFH, DIRL);  PUTNAME;
         IF PROC = 9 THEN
         BEGIN
            PUT32(1);  PUT32(493);  PUT32(0);  PUT32(0);  PUT32(0);  PUT32(0);  PUT32(0)      (* mode 0755, the rest unset *)
         END;
         IF TRANSACT(NFSPORT) THEN
         BEGIN
            N := GET32(0);
            IF N <> 0 THEN NFSERR(N);
            RELEASE
         END
         ELSE SAY('no reply from the server\n')
      END
      ELSE SAY('no such directory\n')
   END
END;

PROCEDURE PUTNAME2;
VAR K : INTEGER;
BEGIN
   PUT32(NM2N);  FOR K := 0 TO NM2N - 1 DO PUT8(NM2[K]);  PADTO4(NM2N)
END;

PROCEDURE DOREN;
VAR N, K : INTEGER;
BEGIN
   IF NT < 3 THEN SAY('usage: ren from to\n')
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      IF (RESOK = 1) AND (NMN > 0) THEN
      BEGIN
         COPYFH(FDIR, FDL, DIRFH, DIRL);
         FOR K := 0 TO NMN - 1 DO NM2[K] := NM[K];
         NM2N := NMN;
         RESOLVE(TS[2], TL[2]);
         IF (RESOK = 1) AND (NMN > 0) THEN
         BEGIN
            RPCCALL(RPCNFS, 3, 14);
            PUTFH(FDIR, FDL);  PUTNAME2;
            PUTFH(DIRFH, DIRL);  PUTNAME;
            IF TRANSACT(NFSPORT) THEN
            BEGIN
               N := GET32(0);
               IF N <> 0 THEN NFSERR(N);
               RELEASE
            END
            ELSE SAY('no reply from the server\n')
         END
         ELSE SAY('no such directory\n')
      END
      ELSE SAY('no such file\n')
   END
END;

PROCEDURE DOCOPY;
VAR OK : BOOLEAN;  K, N : INTEGER;
BEGIN
   IF NT < 3 THEN SAY('usage: copy from to\n')
   ELSE
   BEGIN
      RESOLVE(TS[1], TL[1]);
      OPENSLOT(2);
      IF (RESOK = 1) AND (NMN > 0) AND LOOKUP(2) THEN
      BEGIN
         RESOLVE(TS[2], TL[2]);
         OPENSLOT(3);  SKIND[3] := 2;
         IF (RESOK = 1) AND (NMN > 0) AND CREATE(3) THEN
         BEGIN
            OK := TRUE;
            WHILE OK AND NOT SEOF[2] DO
            BEGIN
               OK := READPIECE(2);
               IF OK THEN
               BEGIN
                  N := SN[2];
                  FOR K := 0 TO N - 1 DO SBUF[3*CHUNK+K] := SBUF[2*CHUNK+K];
                  SN[3] := N;  SP[2] := N;
                  IF N > 0 THEN OK := WRITEPIECE(3)
               END
            END;
            IF NOT OK THEN SAY('copy failed\n')
         END
         ELSE SAY('cannot create the file\n')
      END
      ELSE SAY('cannot open the file\n');
      SKIND[2] := 0;  SKIND[3] := 0
   END
END;

(* ---------------------------------------------------------------- compile *)
PROCEDURE SETNMBASE(E1, E2, E3 : INTEGER);     (* NM := BASE.e1e2e3 (or the file "input" when there is no base) *)
VAR K : INTEGER;
BEGIN
   FOR K := 0 TO BASEN - 1 DO NM[K] := BASE[K];
   NMN := BASEN;  NM[NMN] := 46;  NM[NMN+1] := E1;  NM[NMN+2] := E2;  NM[NMN+3] := E3;
   NMN := NMN + 4
END;

PROCEDURE DOCOMPILE;
VAR K, L, S : INTEGER;  F : BYTES;  FN : INTEGER;  OK : BOOLEAN;
BEGIN
   BASEN := 0;
   IF NT >= 2 THEN
   BEGIN
      L := TL[1];  S := TS[1];
      IF L > 4 THEN
         IF (LINE[S+L-4] = 46) AND (LINE[S+L-3] = 112) AND (LINE[S+L-2] = 97) AND (LINE[S+L-1] = 115) THEN L := L - 4;
      IF L > 40 THEN L := 40;
      FOR K := 0 TO L - 1 DO BASE[K] := LINE[S + K];
      BASEN := L
   END;
   COPYFH(DIRFH, DIRL, CWDFH, CWDL);
   IF BASEN > 0 THEN SETNMBASE(112, 97, 115)
   ELSE BEGIN NM[0] := 105;  NM[1] := 110;  NM[2] := 112;  NM[3] := 117;  NM[4] := 116;  NMN := 5 END;
   IF NOT LOOKUPFH(F, FN) THEN SAY('no such source file\n')
   ELSE
   BEGIN
      OK := TRUE;
      IF (NCOMP > 0) OR (EDITED = 1) THEN
      BEGIN                                    (* the compiler is reloaded from pascimg in the export *)
         COPYFH(DIRFH, DIRL, FHROOT, FHROOTL);
         NM[0] := 112;  NM[1] := 97;  NM[2] := 115;  NM[3] := 99;  NM[4] := 105;  NM[5] := 109;  NM[6] := 103;  NMN := 7;
         OK := LOOKUPFH(F, FN)
      END;
      IF OK THEN
      BEGIN
         NCOMP := NCOMP + 1;  AFTERC := 1;  MB.RES := 1;  LEAVE := 1;  EDITED := 0;
         SAY('compiling ...\n')
      END
      ELSE SAY('the file pascimg (the compiler image) is missing in the export\n')
   END
END;

PROCEDURE COMPRESULT;                          (* after a compile: count the error marks in the listing *)
VAR B, POS, OKP, ERRS : INTEGER;
BEGIN
   COPYFH(DIRFH, DIRL, CWDFH, CWDL);
   IF BASEN > 0 THEN SETNMBASE(108, 115, 116)
   ELSE BEGIN NM[0] := 111;  NM[1] := 117;  NM[2] := 116;  NM[3] := 112;  NM[4] := 117;  NM[5] := 116;  NMN := 6 END;
   OPENSLOT(3);
   IF LOOKUP(3) THEN
   BEGIN
      POS := 0;  OKP := 1;  ERRS := 0;
      REPEAT
         B := NEXTBYTE(3);
         IF B >= 0 THEN
         BEGIN
            IF B = 10 THEN BEGIN POS := 0;  OKP := 1 END
            ELSE
            BEGIN
               IF POS = 0 THEN BEGIN IF B <> 32 THEN OKP := 0 END
               ELSE IF POS < 5 THEN BEGIN IF B <> 42 THEN OKP := 0 END;
               IF (POS = 4) AND (OKP = 1) THEN ERRS := ERRS + 1;
               POS := POS + 1
            END
         END
      UNTIL B < 0;
      SKIND[3] := 0;
      SAY('compile finished, ');  SAYN(ERRS);  SAY(' error line(s) in the listing');
      IF (MB.PL[0] = 2048) AND (MB.PL[1] = 0) THEN SAY(', type run\n') ELSE CRLF
   END
   ELSE SAY('compile finished, no listing\n')
END;

(* ---------------------------------------------------------------- run, and the console for the program that runs *)
PROCEDURE CONPUT(C : INTEGER);                 (* a character of the 1972 set to the UART *)
VAR A : INTEGER;
BEGIN
   A := TOASC[C];
   IF A = 10 THEN CRLF ELSE PUTCH(A)
END;

FUNCTION CONGET(D : INTEGER) : INTEGER;        (* a typed character as the 1972 set has it: 0 = end of line, -1 = end of file *)
VAR K : INTEGER;
BEGIN
   K := GETKEY(1);
   IF K = 4 THEN CONGET := -1
   ELSE IF (K = 13) OR (K = 10) THEN BEGIN CRLF;  CONGET := 0 END
   ELSE BEGIN PUTCH(K);  CONGET := FROMASC[K] END
END;

PROCEDURE DORUN;
BEGIN
   IF NCOMP = 0 THEN SAY('compile first\n')
   ELSE IF MB.PL[0] <> 2048 THEN SAY('the last compile did not finish\n')
   ELSE IF MB.PL[1] <> 0 THEN SAY('the program has errors, see the listing\n')
   ELSE
   BEGIN
      SAY('running ...\n');
      CONSOLE := 1;  AFTERC := 2;  MB.RES := 3;  LEAVE := 1
   END
END;

(*EDIT*)

(* ---------------------------------------------------------------- help and the loop *)
PROCEDURE DOHELP;
BEGIN
   SAY('dir [path]         list a directory (ls)\n');
   SAY('more file          show a file page by page (type); cat: without paging\n');
   SAY('cd [path], pwd     change and show the directory\n');
   SAY('compile [name]     compile name.pas to name.lst, name.cdc, name.pmd\n');
   SAY('                   (no name: input to output, pasclgo, pmdfile)\n');
   SAY('edit file          full screen editor (^S save, ^X leave)\n');
   SAY('run                run the program compile has just made (screen and keyboard are its output and input)\n');
   SAY('copy a b, ren a b, del f, mkdir d, rmdir d, stat f, df\n');
   SAY('info               addresses and the export; net: reconnect; halt\n')
END;

PROCEDURE DOINFO;
BEGIN
   SAY('board ');  SAYIP(MYIP);  SAY('   server ');  SAYIP(SRVIP);  SAY('   export ');
   FOR K := 0 TO EXPN - 1 DO PUTCH(EXPORT[K]);
   CRLF;
   IF DHST = 1 THEN BEGIN SAY('dhcp: address from the server ');  SAYIP(SIDB);  SAY(', lease ');  SAYN(LEASE);  SAY(' s\n') END
   ELSE IF DHST = 2 THEN SAY('dhcp: no answer, using the static address\n')
   ELSE SAY('static address (DIP switch 7)\n')
END;

PROCEDURE DOCMD;
VAR K, IDX : INTEGER;
BEGIN
   MAKECMD;
   IDX := -1;
   FOR K := 0 TO 23 DO IF CMDS[K] = CMDW THEN IDX := K;
   IF (IDX = 1) THEN IDX := 0
   ELSE IF IDX = 3 THEN IDX := 2
   ELSE IF IDX = 9 THEN IDX := 8
   ELSE IF IDX = 11 THEN IDX := 10
   ELSE IF IDX = 13 THEN IDX := 12;
   PAGED := 0;
   IF IDX < 0 THEN SAY('unknown command, try help\n')
   ELSE IF NOT NETOK AND (IDX <> 18) AND (IDX <> 19) AND (IDX <> 20) AND (IDX <> 21) AND (IDX <> 6) THEN SAY('no network: type net\n')
   ELSE
   CASE IDX OF
      0: DODIRCMD;
      2: DOMORE(1);
      4: DOMORE(0);
      5: DOCD;
      6: DOPWD;
      7: DOCOMPILE;
      8: DOCOPY;
      10: DOSIMPLE(12);
      12: DOREN;
      14: DOSIMPLE(9);
      15: DOSIMPLE(13);
      16: DOSTAT;
      17: DODF;
      18: DOHELP;
      19: DOINFO;
      20: BEGIN NETOK := NETSTART(0);  IF NETOK THEN SAY('connected\n') END;
      21: BEGIN MB.RES := 2;  LEAVE := 1 END;
      22: DORUN;
      23: DOEDIT
   END
END;

PROCEDURE INITCMDS;
BEGIN
   CMDS[0] := 'DIR       ';  CMDS[1] := 'LS        ';  CMDS[2] := 'MORE      ';  CMDS[3] := 'TYPE      ';
   CMDS[4] := 'CAT       ';  CMDS[5] := 'CD        ';  CMDS[6] := 'PWD       ';  CMDS[7] := 'COMPILE   ';
   CMDS[8] := 'COPY      ';  CMDS[9] := 'CP        ';  CMDS[10] := 'DEL       ';  CMDS[11] := 'RM        ';
   CMDS[12] := 'REN       ';  CMDS[13] := 'MV        ';  CMDS[14] := 'MKDIR     ';  CMDS[15] := 'RMDIR     ';
   CMDS[16] := 'STAT      ';  CMDS[17] := 'DF        ';  CMDS[18] := 'HELP      ';  CMDS[19] := 'INFO      ';
   CMDS[20] := 'NET       ';  CMDS[21] := 'HALT      ';  CMDS[22] := 'RUN       ';  CMDS[23] := 'EDIT      '
END;

PROCEDURE SHELL;
BEGIN
   LEAVE := 0;  LASTCR := 0;  CLOSEALL;  CONSOLE := 0;
   IF AFTERC = 1 THEN BEGIN AFTERC := 0;  COMPRESULT END
   ELSE IF AFTERC = 2 THEN BEGIN AFTERC := 0;  SAY('\n[program finished]\n') END
   ELSE BEGIN SAY('\nCDC 6000 Pascal on the VC707\n');  DOINFO;  SAY('type help\n') END;
   REPEAT
      PROMPT;  GETLINE;  SPLIT;
      IF NT > 0 THEN DOCMD
   UNTIL LEAVE = 1
END;
