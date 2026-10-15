(* ---------------------------------------------------------------- the screen editor
   edit name: a full screen editor for an ANSI terminal of 24 lines and 80 columns on the UART.  The text is a gap buffer, one byte
   to a word, in MEM (the memory of the compiler, 38K words, which is idle while the shell runs; the compiler is read in again
   by the next compile).  Tabs in a file become spaces when it is read.

   Keys:  arrows, Home, End, PgUp, PgDn (or ^B ^F ^P ^N ^A ^E ^V)    Backspace, Del (or ^D)    ^K kill to the end of the line
          ^S save    ^X leave (twice if there are changes not saved)    ^L draw the screen again      Enter keeps the indent *)

PROCEDURE EDGOTO(R, C : INTEGER);
BEGIN
   PUTCH(27);  PUTCH(91);  SAYN(R);  PUTCH(59);  SAYN(C);  PUTCH(72)
END;

PROCEDURE EDCLREOL;
BEGIN PUTCH(27);  PUTCH(91);  PUTCH(75) END;

FUNCTION EDLEN(D : INTEGER) : INTEGER;
BEGIN EDLEN := EDN - (EDGE - EDGS) END;

FUNCTION EDCHR(P : INTEGER) : INTEGER;         (* the byte at position P of the text *)
BEGIN
   IF P < EDGS THEN EDCHR := MEM[P] ELSE EDCHR := MEM[P + EDGE - EDGS]
END;

FUNCTION EDLSTART(P : INTEGER) : INTEGER;      (* the start of the line that holds position P *)
VAR Q : INTEGER;  DONE : BOOLEAN;
BEGIN
   Q := P;  DONE := FALSE;
   WHILE NOT DONE DO
      IF Q = 0 THEN DONE := TRUE
      ELSE IF EDCHR(Q - 1) = 10 THEN DONE := TRUE
      ELSE Q := Q - 1;
   EDLSTART := Q
END;

(* the primitives: the cursor is the gap; EDCUR is the number of its line, counted from 0 *)
PROCEDURE EDINS(C : INTEGER);
BEGIN
   IF EDGS < EDGE THEN
   BEGIN
      MEM[EDGS] := C;  EDGS := EDGS + 1;
      IF C = 10 THEN EDCUR := EDCUR + 1;
      EDMOD := 1
   END
   ELSE EDMSG := 1
END;

PROCEDURE EDLEFT;
BEGIN
   IF EDGS > 0 THEN
   BEGIN
      MEM[EDGE - 1] := MEM[EDGS - 1];  EDGS := EDGS - 1;  EDGE := EDGE - 1;
      IF MEM[EDGE] = 10 THEN EDCUR := EDCUR - 1
   END
END;

PROCEDURE EDRIGHT;
BEGIN
   IF EDGE < EDN THEN
   BEGIN
      IF MEM[EDGE] = 10 THEN EDCUR := EDCUR + 1;
      MEM[EDGS] := MEM[EDGE];  EDGS := EDGS + 1;  EDGE := EDGE + 1
   END
END;

PROCEDURE EDDELB;                              (* backspace *)
BEGIN
   IF EDGS > 0 THEN
   BEGIN
      EDGS := EDGS - 1;
      IF MEM[EDGS] = 10 THEN EDCUR := EDCUR - 1;
      EDMOD := 1
   END
END;

PROCEDURE EDDELF;                              (* delete the character under the cursor *)
BEGIN
   IF EDGE < EDN THEN BEGIN EDGE := EDGE + 1;  EDMOD := 1 END
END;

FUNCTION EDCOL(D : INTEGER) : INTEGER;         (* the column of the cursor *)
BEGIN EDCOL := EDGS - EDLSTART(EDGS) END;

PROCEDURE EDEOL;                               (* to the end of the line *)
BEGIN
   WHILE (EDGE < EDN) AND (MEM[EDGE] <> 10) DO EDRIGHT
END;

PROCEDURE EDBOL;
BEGIN
   WHILE (EDGS > 0) AND (MEM[EDGS - 1] <> 10) DO EDLEFT
END;

PROCEDURE EDUP;
VAR K, C : INTEGER;
BEGIN
   IF EDCUR > 0 THEN
   BEGIN
      EDBOL;  EDLEFT;  EDBOL;                  (* the start of the line above *)
      C := 0;
      WHILE (C < EDDCOL) AND (EDGE < EDN) AND (MEM[EDGE] <> 10) DO BEGIN EDRIGHT;  C := C + 1 END
   END
END;

PROCEDURE EDDOWN;
VAR C : INTEGER;
BEGIN
   IF EDLEN(0) > EDGS THEN
   BEGIN
      EDEOL;
      IF EDGE < EDN THEN
      BEGIN
         EDRIGHT;  C := 0;
         WHILE (C < EDDCOL) AND (EDGE < EDN) AND (MEM[EDGE] <> 10) DO BEGIN EDRIGHT;  C := C + 1 END
      END
   END
END;

PROCEDURE EDNEWLINE;                           (* Enter: a new line with the indent of this one *)
VAR LS, K, N : INTEGER;
BEGIN
   LS := EDLSTART(EDGS);  N := 0;
   WHILE (LS + N < EDGS) AND (EDCHR(LS + N) = 32) DO N := N + 1;
   EDINS(10);
   FOR K := 1 TO N DO EDINS(32)
END;

PROCEDURE EDKILL;                              (* ^K *)
BEGIN
   IF (EDGE < EDN) AND (MEM[EDGE] = 10) THEN EDDELF
   ELSE WHILE (EDGE < EDN) AND (MEM[EDGE] <> 10) DO EDDELF
END;

(* ---- the keys: an escape sequence becomes a code above 1000 ---- *)
FUNCTION EDKEY(D : INTEGER) : INTEGER;
VAR C, C2, N : INTEGER;
BEGIN
   C := GETKEY(1);
   IF C = 27 THEN
   BEGIN
      C2 := GETKEY(1);
      C := 0;
      IF (C2 = 91) OR (C2 = 79) THEN
      BEGIN
         N := 0;  C2 := GETKEY(1);
         WHILE (C2 >= 48) AND (C2 <= 57) DO BEGIN N := N * 10 + C2 - 48;  C2 := GETKEY(1) END;
         IF C2 = 65 THEN C := 1001
         ELSE IF C2 = 66 THEN C := 1002
         ELSE IF C2 = 67 THEN C := 1003
         ELSE IF C2 = 68 THEN C := 1004
         ELSE IF C2 = 72 THEN C := 1005
         ELSE IF C2 = 70 THEN C := 1006
         ELSE IF C2 = 126 THEN
         BEGIN
            IF (N = 1) OR (N = 7) THEN C := 1005
            ELSE IF (N = 4) OR (N = 8) THEN C := 1006
            ELSE IF N = 5 THEN C := 1007
            ELSE IF N = 6 THEN C := 1008
            ELSE IF N = 3 THEN C := 1009
         END
      END
   END;
   EDKEY := C
END;

(* ---- the screen ---- *)
PROCEDURE EDSTATUS;
VAR K, N : INTEGER;
BEGIN
   EDGOTO(24, 1);
   PUTCH(27);  PUTCH(91);  PUTCH(55);  PUTCH(109);             (* reverse video *)
   N := 0;
   FOR K := 0 TO NM2N - 1 DO BEGIN PUTCH(NM2[K]);  N := N + 1 END;
   IF EDMOD = 1 THEN BEGIN PUTCH(32);  PUTCH(42);  N := N + 2 END;
   SAY('  line ');  SAYN(EDCUR + 1);  SAY(' col ');  SAYN(EDCOL(0) + 1);
   IF EDMSG = 1 THEN SAY('  buffer full')
   ELSE IF EDMSG = 2 THEN SAY('  saved')
   ELSE IF EDMSG = 3 THEN SAY('  not saved: ^X again to leave, ^S to save')
   ELSE IF EDMSG = 4 THEN SAY('  cannot save')
   ELSE SAY('  ^S save  ^X leave');
   EDMSG := 0;
   PUTCH(27);  PUTCH(91);  PUTCH(75);                          (* the rest of the line in reverse too *)
   PUTCH(27);  PUTCH(91);  PUTCH(48);  PUTCH(109)
END;

PROCEDURE EDROW(ROW, P0 : INTEGER);            (* the text of the line that starts at P0 on screen row ROW (1 ...) *)
VAR P, COL, C, LEN : INTEGER;  DONE : BOOLEAN;
BEGIN
   EDGOTO(ROW, 1);
   LEN := EDLEN(0);  P := P0;  COL := 0;  DONE := FALSE;
   WHILE NOT DONE DO
      IF P >= LEN THEN DONE := TRUE
      ELSE
      BEGIN
         C := EDCHR(P);
         IF C = 10 THEN DONE := TRUE
         ELSE
         BEGIN
            IF (COL >= EDHSCR) AND (COL < EDHSCR + 79) THEN PUTCH(C);
            COL := COL + 1;  P := P + 1
         END
      END;
   EDCLREOL
END;

PROCEDURE EDREDRAW;
VAR ROW, P, LEN : INTEGER;  DONE : BOOLEAN;
BEGIN
   LEN := EDLEN(0);  P := EDTOP;
   FOR ROW := 1 TO 23 DO
   BEGIN
      EDROW(ROW, P);
      DONE := FALSE;
      WHILE NOT DONE DO
         IF P >= LEN THEN DONE := TRUE
         ELSE IF EDCHR(P) = 10 THEN BEGIN P := P + 1;  DONE := TRUE END
         ELSE P := P + 1
   END
END;

(* the cursor into view: scroll the lines and the columns; 1 if the whole screen must be drawn again *)
FUNCTION EDVIEW(D : INTEGER) : INTEGER;
VAR FULL, COL, R : INTEGER;
BEGIN
   FULL := 0;
   COL := EDCOL(0);
   IF COL < EDHSCR THEN BEGIN EDHSCR := COL - 20;  IF EDHSCR < 0 THEN EDHSCR := 0;  FULL := 1 END
   ELSE IF COL >= EDHSCR + 79 THEN BEGIN EDHSCR := COL - 60;  FULL := 1 END;
   R := EDCUR - EDTOPL;
   IF R < 0 THEN
   BEGIN
      EDTOP := EDLSTART(EDGS);  EDTOPL := EDCUR;  FULL := 1
   END
   ELSE IF R > 22 THEN
   BEGIN
      WHILE EDCUR - EDTOPL > 22 DO
      BEGIN
         WHILE (EDTOP < EDLEN(0)) AND (EDCHR(EDTOP) <> 10) DO EDTOP := EDTOP + 1;
         EDTOP := EDTOP + 1;  EDTOPL := EDTOPL + 1
      END;
      FULL := 1
   END;
   EDVIEW := FULL
END;

PROCEDURE EDSHOW(FULL : INTEGER);              (* after a key: draw what changed and put the cursor in place *)
VAR F : INTEGER;
BEGIN
   F := FULL;
   IF EDVIEW(0) = 1 THEN F := 1;
   IF F = 1 THEN EDREDRAW
   ELSE IF F = 2 THEN EDROW(EDCUR - EDTOPL + 1, EDLSTART(EDGS));
   EDSTATUS;
   EDGOTO(EDCUR - EDTOPL + 1, EDCOL(0) - EDHSCR + 1)
END;

(* ---- files ---- *)
FUNCTION EDLOAD(D : INTEGER) : BOOLEAN;        (* NM in DIRFH into the buffer; a missing file is an empty one *)
VAR B, COL, K : INTEGER;  OK : BOOLEAN;
BEGIN
   OK := TRUE;  EDGS := 0;  EDGE := EDN;  EDCUR := 0;
   OPENSLOT(3);
   IF LOOKUP(3) THEN
   BEGIN
      REPEAT
         B := NEXTBYTE(3);
         IF B >= 0 THEN
         BEGIN
            IF B = 9 THEN BEGIN COL := EDGS - EDLSTART(EDGS);  FOR K := 1 TO 8 - COL MOD 8 DO EDINS(32) END
            ELSE IF B = 13 THEN B := B
            ELSE IF (B = 10) OR ((B >= 32) AND (B < 127)) THEN EDINS(B)
            ELSE EDINS(63);
            IF EDMSG = 1 THEN OK := FALSE
         END
      UNTIL (B < 0) OR NOT OK
   END;
   SKIND[3] := 0;
   WHILE EDGS > 0 DO EDLEFT;                  (* the cursor to the start *)
   EDMOD := 0;  EDLOAD := OK
END;

FUNCTION EDSAVE(D : INTEGER) : BOOLEAN;
VAR P, LEN, K : INTEGER;  OK : BOOLEAN;
BEGIN
   COPYFH(DIRFH, DIRL, FDIR, FDL);
   FOR K := 0 TO NM2N - 1 DO NM[K] := NM2[K];
   NMN := NM2N;
   OPENSLOT(3);  SKIND[3] := 2;
   OK := CREATE(3);
   IF OK THEN
   BEGIN
      LEN := EDLEN(0);  P := 0;
      WHILE OK AND (P < LEN) DO
      BEGIN
         OK := PUTBYTE(3, EDCHR(P));
         P := P + 1
      END;
      IF OK THEN OK := WRITEPIECE(3)
   END;
   SKIND[3] := 0;
   IF OK THEN EDMOD := 0;
   EDSAVE := OK
END;

PROCEDURE DOEDIT;
VAR K, C, FULL, DONE : INTEGER;
BEGIN
   IF NT < 2 THEN SAY('usage: edit file\n')
   ELSE
   BEGIN
      IF RAMMODE THEN RAMNAME(1) ELSE RESOLVE(TS[1], TL[1]);
      IF (RESOK = 0) OR (NMN = 0) THEN SAY('no such directory\n')
      ELSE
      BEGIN
         COPYFH(FDIR, FDL, DIRFH, DIRL);
         FOR K := 0 TO NMN - 1 DO NM2[K] := NM[K];
         NM2N := NMN;
         EDN := 38900;  EDHSCR := 0;  EDTOP := 0;  EDTOPL := 0;  EDMSG := 0;  EDQUIT := 0;  EDDCOL := 0;
         MB.PL[15] := 1;  EDITED := 1;                      (* the compiler's memory is used: the run time reads it in again *)
         IF NOT EDLOAD(0) THEN SAY('file too large for the editor\n')
         ELSE
         BEGIN
            PUTCH(27);  PUTCH(91);  PUTCH(50);  PUTCH(74);                   (* clear the screen *)
            EDSHOW(1);
            DONE := 0;
            REPEAT
               C := EDKEY(0);  FULL := 0;
               IF C = 24 THEN                                     (* ^X *)
               BEGIN
                  IF (EDMOD = 1) AND (EDQUIT = 0) THEN BEGIN EDMSG := 3;  EDQUIT := 1 END
                  ELSE DONE := 1
               END
               ELSE
               BEGIN
                  EDQUIT := 0;
                  IF C = 19 THEN BEGIN IF EDSAVE(0) THEN EDMSG := 2 ELSE EDMSG := 4 END
                  ELSE IF (C = 1003) OR (C = 6) THEN BEGIN EDRIGHT;  EDDCOL := EDCOL(0) END
                  ELSE IF (C = 1004) OR (C = 2) THEN BEGIN EDLEFT;  EDDCOL := EDCOL(0) END
                  ELSE IF (C = 1001) OR (C = 16) THEN EDUP
                  ELSE IF (C = 1002) OR (C = 14) THEN EDDOWN
                  ELSE IF (C = 1005) OR (C = 1) THEN BEGIN EDBOL;  EDDCOL := 0 END
                  ELSE IF (C = 1006) OR (C = 5) THEN BEGIN EDEOL;  EDDCOL := EDCOL(0) END
                  ELSE IF C = 1007 THEN BEGIN FOR K := 1 TO 20 DO EDUP END
                  ELSE IF (C = 1008) OR (C = 22) THEN BEGIN FOR K := 1 TO 20 DO EDDOWN END
                  ELSE IF C = 12 THEN FULL := 1
                  ELSE IF (C = 127) OR (C = 8) THEN
                  BEGIN
                     IF (EDGS > 0) AND (MEM[EDGS - 1] = 10) THEN FULL := 1 ELSE FULL := 2;
                     EDDELB;  EDDCOL := EDCOL(0)
                  END
                  ELSE IF (C = 1009) OR (C = 4) THEN
                  BEGIN
                     IF (EDGE < EDN) AND (MEM[EDGE] = 10) THEN FULL := 1 ELSE FULL := 2;
                     EDDELF
                  END
                  ELSE IF C = 11 THEN BEGIN FULL := 1;  EDKILL END
                  ELSE IF (C = 13) OR (C = 10) THEN BEGIN EDNEWLINE;  FULL := 1;  EDDCOL := EDCOL(0) END
                  ELSE IF C = 9 THEN BEGIN FOR K := 1 TO 3 - EDCOL(0) MOD 3 DO EDINS(32);  FULL := 2;  EDDCOL := EDCOL(0) END
                  ELSE IF (C >= 32) AND (C < 127) THEN BEGIN EDINS(C);  FULL := 2;  EDDCOL := EDCOL(0) END;
                  IF (C = 1001) OR (C = 1002) OR (C = 1007) OR (C = 1008) THEN FULL := 0
               END;
               IF DONE = 0 THEN EDSHOW(FULL)
            UNTIL DONE = 1;
            PUTCH(27);  PUTCH(91);  PUTCH(50);  PUTCH(74);  EDGOTO(1, 1)
         END
      END
   END
END;
